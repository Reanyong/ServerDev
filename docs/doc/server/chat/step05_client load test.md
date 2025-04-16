## 🎯 목표

- WebSocket 서버에 대해 다수의 클라이언트를 시뮬레이션하여 부하 테스트 수행
- 메시지 송수신 처리 성능 측정 (Throughput)
- 멀티스레드 기반으로 병렬 클라이언트 테스트 실행

---

## ⚙️ 주요 기능

-  `Client` 클래스 생성으로 WebSocket 연결/전송/수신/종료 처리
-  클라이언트 수 지정 (`CLIENT_COUNT`)
-  클라이언트별 메시지 수 지정 (`MESSAGE_COUNT`)
-  각 클라이언트는 고유 닉네임을 서버에 전달
-  서버로 메시지 전송 후 응답 수신 (Echo 확인)
-  `std::thread`를 이용한 병렬 메시지 송신 처리
-  테스트 수행 시간 측정 및 메시지 처리량 계산 (messages/sec)

---

## 🧪 테스트 방법

1. 서버가 실행 중인지 확인 (8080 포트)
2. Visual Studio 또는 콘솔에서 본 테스트 프로그램 빌드 및 실행
3. CLI 출력 결과 확인:
    - 전체 수행 시간
    - 초당 메시지 처리량 (Throughput)

---

## ⚙️ 테스트 환경 설정

```cpp
const int CLIENT_COUNT = 100;        // 동시에 연결할 클라이언트 수
const int MESSAGE_COUNT = 50;        // 각 클라이언트가 보낼 메시지 수
```

💡 총 전송 메시지 수: `CLIENT_COUNT * MESSAGE_COUNT`

---

## 💬 출력 예시

```
Test completed in 9.85 seconds
Throughput: 5078.35 messages/sec
```

---

## 📌 주의사항

- 서버는 반드시 멀티 클라이언트 및 멀티스레드 환경에 대응 가능해야 함 (Step04 이상)
- 서버와 클라이언트는 동일 머신에서 실행하는 경우 CPU 부하에 주의
- `rand()` 기반 지연 추가로 서버에 과도한 폭주 방지 (`100~500ms` 랜덤 지연)
    

---

## 📂 파일 구조 예시

```
/ServerDev/
  ├── src/step05/load_test_client.cpp             // 이 코드
  └── doc/server/chat/step05_client_load_test.md // 이 문서
```

---