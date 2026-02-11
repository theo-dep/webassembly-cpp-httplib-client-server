#pragma once

#include <cstddef>

void SHA1(char *hash_out, const char *str, int len);

void initWebSocketSendLock();

void wait_websocket_client(int client_fd);
