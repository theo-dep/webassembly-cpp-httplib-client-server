#pragma once

void initWebSocketRegistry();
void connectWebSocketClient(int client_fd, const char* secWebSocketHeader);
