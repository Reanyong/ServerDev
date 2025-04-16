## 🎯 목표
Boost.Beast와 Boost.Asio를 활용하여 비동기 + 멀티스레드 기반의 고성능 WebSocket 채팅 서버를 구현합니다. 동시 다중 클라이언트 연결을 처리할 수 있으며, 각 클라이언트 메시지를 비동기로 수신/응답합니다.

---

## ⚙️ 주요 기능
- [x] WebSocket 서버 동작을 위한 `boost::beast` 기반 구조 구현
- [x] 클라이언트 연결 요청을 비동기적으로 수락 (`async_accept`)
- [x] 각 연결에 대해 `Session` 클래스로 관리 (RAII + enable_shared_from_this)
- [x] 클라이언트 메시지를 `async_read`로 비동기 수신
- [x] 수신된 메시지를 그대로 다시 `async_write`로 Echo 응답
- [x] UTF-8 인코딩된 한글 메시지를 `wstring`으로 변환해 콘솔 출력
- [x] 콘솔 출력에 `WriteConsoleW` API를 사용하여 깨짐 없이 한글 출력
- [x] `mutex`를 활용해 스레드 안전한 콘솔 로그 구현 (`ConsoleOut`, `ConsoleErr`)
- [x] `hardware_concurrency()` 기반 다중 스레드 구성으로 성능 향상
- [x] 각 스레드에서 `io_context.run()`을 통해 비동기 작업 실행
---

## 🧪 테스트 방법

1. Visual Studio 2022에서 `multithread_websocket_echo.cpp`빌드
2. `test/multithread_websocket_test.html` 실행 (WebSocket 접속)
3. 여러 브라우저 창에서 동시에 접속 후 메시지 송수신 테스트

---

## 💬 예시 출력

```
MultiThread Websocket Start!! - Port 8080
시스템 CPU 코어/스레드 수: 12
[Server] 클라이언트 연결됨 [Thread ID: 1362391283]
[Session] WebSocket 연결 성공 [Thread ID: 1362391283]
[Recv] 안녕하세요
```

---

## 🧠 구현 요소 정리

| 구성 요소                          | 설명                                         |
| ------------------------------ | ------------------------------------------ |
| `Session` 클래스                  | 각 클라이언트 WebSocket 세션 담당 (비동기 read/write)   |
| `Server` 클래스                   | 클라이언트 연결 수락 및 스레드 풀 구성                     |
| `utf8_to_wstring()`            | UTF-8 문자열을 wchar_t로 변환 (한글 처리)             |
| `ConsoleOut()`, `ConsoleErr()` | 스레드 안전한 콘솔 출력 처리 (mutex, WriteConsoleW 사용) |

---

## 📌 참고 사항

- 클라이언트 메시지를 브로드캐스트하지 않고, 각 세션에 대해 Echo 응답만 수행합니다.
    
- 추후 JSON 메시지 처리, 인증, DB 연동 등을 통해 확장 예정입니다.