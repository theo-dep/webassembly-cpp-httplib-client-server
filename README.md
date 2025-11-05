# webassembly-cpp-httplib-client-server

Client and Server demonstration using Emscripten and cpp-httplib through WebSocket bridge.

## Emscripten

Download and install latest [Emscripten](https://emscripten.org/docs/getting_started/downloads.html) (tested with 4.0.18).

## Build with CMake

```
cmake --preset all
cmake --build build
```

Then run `./build/Debug/server` and look at http://localhost:8080/.

## Links

- [Full POSIX Sockets over WebSocket Proxy Server](https://emscripten.org/docs/porting/networking.html#full-posix-sockets-over-websocket-proxy-server)
- [cpp-httplib issue 2261](https://github.com/yhirose/cpp-httplib/issues/2261)
