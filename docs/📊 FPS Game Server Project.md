> C++ 기반 대규모 전장 FPS 게임 서버 개발 프로젝트 (with PostgreSQL, Redis, Kafka, AWS)

---

## 🌐 Project Overview

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

## 📈 Tech Stack

| 기능                  | 기술 스택                        | 설명            |
| ------------------- | ---------------------------- | ------------- |
| API Server          | C++ + gRPC                   | REST/gRPC API |
| Game Session Server | C++ + Boost.Asio UDP         | 저지연 게임 동기화    |
| Event Stream        | C++ + Kafka or Redis Pub/Sub | 실시간 이벤트 처리    |
| Chat Server         | C++ + Boost WebSocket        | 채팅 매칭 시스템     |
| Database            | C++ + PostgreSQL             | 유저, 점수 데이터    |
| Session Cache       | C++ + Redis                  | 서버 세션 캐시드     |
| File Storage        | AWS S3 + C++ SDK             | 정적 프로파일 저장    |
| Scaling             | AWS ALB + EC2 Auto Scaling   | 수평 확장         |
| Serverless          | AWS Lambda + C++             | 경량 작업 처리      |

---

## 📏 System Architecture

### DB ERD (PostgreSQL)
- Users
- Sessions
- Roles
- Scores
- Rankings
- MMR_Calculations
- Chats & Messages
- GameSessions
- BattleLogs

### AWS Infra (Free Tier)
- EC2 (API 서버)
- Aurora PostgreSQL (RDS)
- ElastiCache Redis
- Kafka (스트리밍)
- API Gateway + Lambda
- S3 (파일저장)
- ALB + Auto Scaling

---

## 🛠️ Development Plan

1. DB 설계 (ERD 기반)
2. UDP 기반 Game Session Server 개발
3. gRPC 기반 API Server 구축
4. Redis/Kafka 비동기 처리
5. WebSocket 채팅 서버
6. AWS 서비스 연동 (Gamelift / Lambda 등)
7. 부하 테스트 & 시뮬레이션 자동화

---

## 🌟 목표

- 대규모 FPS 전장 시스템의 서버 구조 체험
- PostgreSQL/Redis/Kafka를 활용한 복합 아키텍처 구축
- AWS 무료 티어를 기반으로 확장형 서버 설계
- 포트폴리오에 어필 가능한 신입 서버 엔지니어 경험 축적
