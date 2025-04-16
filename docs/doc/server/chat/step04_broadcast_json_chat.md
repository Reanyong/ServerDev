## 🎯 목표

- JSON 기반 메시지를 사용하는 구조화된 WebSocket 채팅 서버 구현
- `/nick` 명령어로 사용자 닉네임 변경 기능 추가
- 유저 입장/퇴장/일반 채팅/시스템 메시지 분리 처리
- 멀티스레드 + 비동기 + 안전한 메시지 브로드캐스트 구현
    

---

## ⚙️ 주요 기능

-  `nlohmann::json` 기반 메시지 직렬화 / 역직렬화
-  `ChatMessage` 구조체 및 `MessageType` 열거형 도입
-  `/nick [새닉네임]` 명령어 처리 구현
-  채팅방 입장/퇴장/일반 채팅 메시지 처리
-  `Session` 클래스에서 메시지 큐 + 전송 흐름 관리
-  `ChatRoom` 클래스에서 참가자 목록 및 브로드캐스트 관리
-  콘솔 UTF-8 한글 출력 및 멀티스레드 안전 로그 처리 (`ConsoleOut`, `ConsoleErr`)
-  `write_mutex_` 및 메시지 큐(`std::queue`)를 이용한 순차 메시지 전송 보장

---

## 🧪 테스트 방법

1. Visual Studio 2022에서 프로젝트 `broadcast_websocket.cpp` 빌드
2. `test/broadcast_websocket_test.html` 클라이언트 실행 (WebSocket 8080 연결)
3. 여러 브라우저 창에서 접속 후 메시지 송수신 테스트
4. `/nick 새닉네임` 명령어로 닉네임 변경 테스트

---

## 💬 메시지 예시 (JSON)

```json
{
  "type": 2,
  "nickname": "User1",
  "content": "안녕하세요",
  "timestamp": "2025-04-01 14:20:11"
}
```

| 타입  | 설명               |
| --- | ---------------- |
| 0   | JOIN (입장 메시지)    |
| 1   | LEAVE (퇴장 메시지)   |
| 2   | CHAT (일반 채팅)     |
| 3   | SYSTEM (시스템 메시지) |

---

## 🧠 구현 요소 정리

| 컴포넌트                           | 설명                              |
| ------------------------------ | ------------------------------- |
| `ChatMessage`                  | 메시지 타입 정의 및 JSON 변환 기능 포함       |
| `Session`                      | 클라이언트 소켓 연결 처리, 메시지 수신/전송 로직 포함 |
| `ChatRoom`                     | 전체 참가자 관리, 메시지 브로드캐스트 처리        |
| `ConsoleOut`, `ConsoleErr`     | 멀티스레드 안전한 콘솔 출력 도우미             |
| `write_queue_`, `write_mutex_` | 메시지 순차 전송 및 race condition 방지   |

---