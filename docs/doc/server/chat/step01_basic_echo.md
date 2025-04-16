## 🎯 목표
Boost.Beast를 사용하여 단일 클라이언트와 WebSocket으로 통신하는 기본 에코 서버를 구현합니다.  
클라이언트가 보낸 메시지를 그대로 되돌려 보내는 구조로, WebSocket 서버의 동작 원리를 학습하는 데 목적이 있습니다.

---

## ⚙️ 주요 기능

-  WebSocket 핸드셰이크 처리
-  단일 클라이언트 연결 유지
-  수신 메시지를 그대로 응답 (echo)
-  클라이언트 연결 종료 처리

---

## 🧪 테스트 방법

1. Visual Studio 2022에서 `echo_websocket.cpp` 실행
2. `test/server_chat/echo_websocket_test.html` 파일을 브라우저에서 열기
3. WebSocket 서버에 연결 후 메시지를 입력
4. 서버로부터 동일한 메시지를 수신하면 성공

---
## 📌 참고 사항

- Boost 1.82 이상 권장
- 테스트 클라이언트는 WebSocket 표준 API 기반 (HTML5 호환 브라우저 필수)
- 아직 멀티 클라이언트, 인증, JSON 메시지 등은 구현되어 있지 않음

---

> 작성자: [Reanyong]  
> 작성일: 2025-04-01