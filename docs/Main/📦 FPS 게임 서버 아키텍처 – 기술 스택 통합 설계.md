## 🌐 프로젝트 개요

### ✅ 목적

- C++로 고성능 백엔드 서버 구현
    
- AWS 무료 티어 기반 인프라 구성
    
- 클라이언트 없이도 시뮬레이션 가능한 서버 구조 설계
    

### ✅ 주요 기능

- 대기수 전장 시스템 (Battlefield 스타일)
    
- 실시간 데미지 처리 & 동기화
    
- Region 부하 분산 관리
    
- 실시간 채팅 시스템
    
- Redis / Kafka 기반 비동기 이벤트 처리
    
- PostgreSQL 기반 데이터 저장
    

---

## 📈 통합 기술 스택 설계

| 기능                  | 기술 스택                                  | 설명                          |
| ------------------- | -------------------------------------- | --------------------------- |
| API Server          | C++ + RESTful API (Boost.Beast) + gRPC | 유저 인증, 랭킹, 게임 상태 등 요청/응답 처리 |
| Game Session Server | C++ + Boost.Asio UDP + gRPC            | 저지연 전장 동기화, 실시간 상태 처리       |
| Chat Server         | C++ + Boost.Beast WebSocket + Asio     | 실시간 채팅 메시지 처리 및 브로드캐스트      |
| DB 처리               | C++ + libpqxx + PostgreSQL             | 유저, 전투, 채팅 기록 저장            |
| 캐시/이벤트 처리           | Redis / Kafka                          | 실시간 이벤트 브로드캐스트 및 캐싱         |
| 파일 저장               | AWS S3 + C++ SDK                       | 프로필, 리플레이 저장 등 정적 자원        |
| 서버리스 처리             | AWS Lambda + C++                       | 랭킹 계산, 리포트 생성 등 경량 작업       |

---

## 📁 서버 구성 요약

```
[Client]
   │
   ├── RESTful API ───────────────> [API Server]
   │                                      │
   ├── WebSocket (Chat) ───────────────> [Chat Server]
   │                                      │
   └── gRPC (게임 상태) ───────────────> [Game Session Server]
                                          │
        ┌─────→ PostgreSQL (DB)
        └─────→ Redis / Kafka (Pub/Sub)
```

---

## 📚 기능별 기술 매핑

|   |   |   |
|---|---|---|
|기능|기술|설명|
|유저 로그인/회원가입|RESTful API (Boost.Beast)|HTTP 기반 인증 처리|
|채팅 연결 및 메시지 송수신|WebSocket + Boost.Beast|실시간 채팅 처리|
|게임 중 상태 동기화|gRPC + Boost.Asio UDP|위치/데미지/MMR 등 실시간 동기화|
|채팅/게임 로그 저장|PostgreSQL + libpqxx|서버 이벤트 기록 저장|
|이벤트 브로드캐스트|Redis Pub/Sub / Kafka|상태 동기화, 채팅 확장성 지원|
|랭킹/점수 조회|RESTful API|클라이언트 요청 처리용|
|관리자 기능 (밴, 추방 등)|RESTful API|관리자 제어 기능 구현|

---

## 🛠️ 개발 순서 제안

1. PostgreSQL 기반 ERD 설계
    
2. RESTful API 서버 구축 (회원가입, 로그인, 랭킹)
    
3. WebSocket 기반 Chat Server 구현 (Boost.Beast)
    
4. gRPC 기반 Game Session Server 구현
    
5. Boost.Asio 기반 UDP 전투 동기화 처리
    
6. Redis/Kafka 연동 통한 비동기 이벤트 처리 구조 구축
    
7. 전체 통합 및 테스트, 이후 AWS에 배포
    

---

## 📏 DB ERD (PostgreSQL)

- Users
    
- Sessions
    
- Roles
    
- Scores
    
- Rankings
    
- MMR_Calculations
    
- Chats & Messages
    
- GameSessions
    
- BattleLogs
    

---

## ☁️ AWS Infra (Free Tier)

- EC2 (API 서버)
    
- Aurora PostgreSQL (RDS)
    
- ElastiCache Redis
    
- Kafka (스트리밍)
    
- API Gateway + Lambda
    
- S3 (파일저장)
    
- ALB + Auto Scaling
    

---

## 🌟 목표

- gRPC, RESTful API, WebSocket 등 혼합 아키텍처 설계 능력 향상
    
- Boost 기반 네트워크 서버 구축 경험
    
- PostgreSQL/Redis/Kafka를 활용한 데이터/이벤트 처리 능력 강화
    
- AWS 인프라 운영 및 자동화 경험 습득
    
- 포트폴리오에 어필 가능한 대규모 실시간 서버 구조 설계