#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <memory.h>
#include <vector>
#include <sys/types.h>
#include <inttypes.h>

#include "posix_sockets.h"
#include "threads.h"
#include "sha1.h"
#include "websocket_to_posix_proxy.h"
#include "socket_registry.h"

// #define PROXY_DEEP_DEBUG

static const unsigned char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static void base64_encode(void *dst, const void *src, size_t len) { // thread-safe, re-entrant
  assert(dst != src);
  unsigned int *d = (unsigned int *)dst;
  const unsigned char *s = (const unsigned char*)src;
  const unsigned char *end = s + len;
  while (s < end) {
    uint32_t e = *s++ << 16;
    if (s < end) e |= *s++ << 8;
    if (s < end) e |= *s++;
    *d++ = b64[e >> 18] | (b64[(e >> 12) & 0x3F] << 8) | (b64[(e >> 6) & 0x3F] << 16) | (b64[e & 0x3F] << 24);
  }
  for (size_t i = 0; i < (3 - (len % 3)) % 3; i++) ((char *)d)[-1-i] = '=';
}

#define BUFFER_SIZE 1024

// Sends WebSocket handshake back to the given WebSocket connection.
void SendHandshake(int fd, const char* secWebSocketHeader) {
  const char webSocketGlobalGuid[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"; // 36 characters long
  char key[128+sizeof(webSocketGlobalGuid)];
  strcpy(key, secWebSocketHeader);
  strcat(key, webSocketGlobalGuid);

  char sha1[21];
  printf("hashing key: \"%s\"\n", key);
  SHA1(sha1, key, (int)strlen(key));

  char handshakeMsg[] =
    "HTTP/1.1 101 Switching Protocols\r\n"
    "Upgrade: websocket\r\n"
    "Connection: Upgrade\r\n"
    "Sec-WebSocket-Accept: 0000000000000000000000000000\r\n"
    "\r\n";

  base64_encode(strstr(handshakeMsg, "Sec-WebSocket-Accept: ") + strlen("Sec-WebSocket-Accept: "), sha1, 20);

  int err = send(fd, handshakeMsg, (int)strlen(handshakeMsg), 0);
  if (err < 0) {
    fprintf(stderr, "Client write failed\n");
    return;
  }

  printf("Sent handshake:\n%s\n", handshakeMsg);
}

// Validates if the given, possibly partially received WebSocket message has
// enough bytes to contain a full WebSocket header.
static bool WebSocketHasFullHeader(uint8_t *data, uint64_t obtainedNumBytes) {
  if (obtainedNumBytes < 2) return false;
  uint64_t expectedNumBytes = 2;
  WebSocketMessageHeader *header = (WebSocketMessageHeader *)data;
  if (header->mask) expectedNumBytes += 4;
  switch (header->payloadLength) {
    case 127: return expectedNumBytes += 8; break;
    case 126: return expectedNumBytes += 2; break;
    default: break;
  }
  return obtainedNumBytes >= expectedNumBytes;
}

// Computes the total number of bytes that the given WebSocket message will take
// up.
uint64_t WebSocketFullMessageSize(uint8_t *data, uint64_t obtainedNumBytes) {
  assert(WebSocketHasFullHeader(data, obtainedNumBytes));

  uint64_t expectedNumBytes = 2;
  WebSocketMessageHeader *header = (WebSocketMessageHeader *)data;
  if (header->mask) expectedNumBytes += 4;
  switch (header->payloadLength) {
    case 127: return expectedNumBytes += 8 + ntoh64(*(uint64_t*)(data+2)); break;
    case 126: return expectedNumBytes += 2 + ntohs(*(uint16_t*)(data+2)); break;
    default: expectedNumBytes += header->payloadLength; break;
  }
  return expectedNumBytes;
}

// Tests the structure integrity of the websocket message length.
bool WebSocketValidateMessageSize(uint8_t *data, uint64_t obtainedNumBytes) {
  uint64_t expectedNumBytes = WebSocketFullMessageSize(data, obtainedNumBytes);

  if (expectedNumBytes != obtainedNumBytes) {
    printf("Corrupt WebSocket message size! (got %" PRIu64 " bytes, expected %" PRIu64 " bytes)\n", obtainedNumBytes, expectedNumBytes);
    printf("Received data:");
    for (size_t i = 0; i < obtainedNumBytes; ++i)
      printf(" %02X", data[i]);
    printf("\n");
  }
  return expectedNumBytes == obtainedNumBytes;
}

uint64_t WebSocketMessagePayloadLength(uint8_t *data, uint64_t numBytes) {
  WebSocketMessageHeader *header = (WebSocketMessageHeader *)data;
  switch (header->payloadLength) {
    case 127: return ntoh64(*(uint64_t*)(data+2));
    case 126: return ntohs(*(uint16_t*)(data+2));
    default: return header->payloadLength;
  }
}

uint32_t WebSocketMessageMaskingKey(uint8_t *data, uint64_t numBytes) {
  WebSocketMessageHeader *header = (WebSocketMessageHeader *)data;
  if (!header->mask) return 0;
  switch (header->payloadLength) {
    case 127: return *(uint32_t*)(data+10);
    case 126: return *(uint32_t*)(data+4);
    default: return *(uint32_t*)(data+2);
  }
}

uint8_t *WebSocketMessageData(uint8_t *data, uint64_t numBytes) {
  WebSocketMessageHeader *header = (WebSocketMessageHeader *)data;
  data += 2; // Two bytes of fixed size header
  if (header->mask) data += 4; // If there is a masking key present in the header, that takes up 4 bytes
  switch (header->payloadLength) {
    case 127: return data + 8; // 64-bit length
    case 126: return data + 2; // 16-bit length
    default: return data; // 7-bit length that was embedded in fixed size header.
  }
}

const char *WebSocketOpcodeToString(int opcode) {
  static const char *opcodes[] = {
    "continuation frame (0x0)",
    "text frame (0x1)",
    "binary frame (0x2)",
    "reserved(0x3)",
    "reserved(0x4)",
    "reserved(0x5)",
    "reserved(0x6)",
    "reserved(0x7)",
    "connection close (0x8)",
    "ping (0x9)",
    "pong (0xA)",
    "reserved(0xB)",
    "reserved(0xC)",
    "reserved(0xD)",
    "reserved(0xE)",
    "reserved(0xF)"
  };
  return opcodes[opcode];
}

void DumpWebSocketMessage(uint8_t *data, uint64_t numBytes) {
  bool goodMessageSize = WebSocketValidateMessageSize(data, numBytes);
  if (!goodMessageSize)
    return;

  WebSocketMessageHeader *header = (WebSocketMessageHeader *)data;
  uint64_t payloadLength = WebSocketMessagePayloadLength(data, numBytes);
  uint8_t *payload = WebSocketMessageData(data, numBytes);

  printf("Received: FIN: %d, opcode: %s, mask: 0x%08X, payload length: %" PRIu64 " bytes, unmasked payload:", header->fin, WebSocketOpcodeToString(header->opcode),
    WebSocketMessageMaskingKey(data, numBytes), payloadLength);
  for (uint64_t i = 0; i < payloadLength; ++i) {
    if (i%16 == 0) printf("\n");
    if (i%8==0) printf(" ");
    printf(" %02X", payload[i]);
    if (i >= 63 && payloadLength > 64) {
      printf("\n   ... (%" PRIu64 " more bytes)", payloadLength-i);
      break;
    }
  }
  printf("\n");
}

void connectWebSocketClient(int client_fd, const char* secWebSocketHeader) {
  SendHandshake(client_fd, secWebSocketHeader);

  std::vector<uint8_t> fragmentData;
  char buf[BUFFER_SIZE];

  bool connectionAlive = true;
  while (connectionAlive) {
    int read = recv(client_fd, buf, BUFFER_SIZE, 0);

    if (!read) break; // done reading
    if (read < 0) {
      fprintf(stderr, "Client read failed\n");
      return;
    }

#ifdef PROXY_DEEP_DEBUG
    printf("Received:");
    for (int i = 0; i < read; ++i) {
      printf(" %02X", ((unsigned char*)buf)[i]);
    }
    printf("\n");
//    printf("In text:\n%s\n", buf);
#endif

#ifdef PROXY_DEEP_DEBUG
    printf("Have %d+%d==%d bytes now in queue\n", (int)fragmentData.size(), (int)read, (int)(fragmentData.size()+read));
#endif
    fragmentData.insert(fragmentData.end(), buf, buf+read);

    // Process received fragments until there is not enough data for a full message
    while (!fragmentData.empty()) {
      bool hasFullHeader = WebSocketHasFullHeader(&fragmentData[0], fragmentData.size());
      if (!hasFullHeader) {
#ifdef PROXY_DEEP_DEBUG
        printf("(not enough for a full WebSocket header)\n");
#endif
        break;
      }
      uint64_t neededBytes = WebSocketFullMessageSize(&fragmentData[0], fragmentData.size());
      if (fragmentData.size() < neededBytes) {
#ifdef PROXY_DEEP_DEBUG
        printf("(not enough for a full WebSocket message, needed %d bytes)\n", (int)neededBytes);
#endif
        break;
      }

      WebSocketMessageHeader *header = (WebSocketMessageHeader *)&fragmentData[0];
      uint64_t payloadLength = WebSocketMessagePayloadLength(&fragmentData[0], neededBytes);
      uint8_t *payload = WebSocketMessageData(&fragmentData[0], neededBytes);

      // Unmask payload
      if (header->mask)
        WebSocketMessageUnmaskPayload(payload, payloadLength, WebSocketMessageMaskingKey(&fragmentData[0], neededBytes));

#ifdef PROXY_DEEP_DEBUG
        DumpWebSocketMessage(&fragmentData[0], neededBytes);
#endif

      switch (header->opcode) {
      case 0x02: /*binary message*/ ProcessWebSocketMessage(client_fd, payload, payloadLength); break;
      case 0x08: connectionAlive = false; break;
      default:
        fprintf(stderr, "Unknown WebSocket opcode received %x!\n", header->opcode);
        connectionAlive = false; // Kill connection
        break;
      }

      fragmentData.erase(fragmentData.begin(), fragmentData.begin() + (ptrdiff_t)neededBytes);
#ifdef PROXY_DEEP_DEBUG
      printf("Cleared used bytes, got %d left in fragment queue.\n", (int)fragmentData.size());
#endif
    }
  }
}

void initWebSocketRegistry() {
  InitWebSocketRegistry();
}
