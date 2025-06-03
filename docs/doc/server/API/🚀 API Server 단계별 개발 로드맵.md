## 📋 전체 개발 플로우

```
Phase 1 → Phase 2 → Phase 3 → Phase 4 → Phase 5
    ↓
코드 리팩토링 & 최적화
    ↓
채팅서버 연동 테스트
    ↓
통합 시스템 완성! 🎉
```

---

## 🎯 Phase 1: HTTP 서버 기초 (1주)

### 🎯 **목표**: Boost.Beast로 간단한 HTTP 서버 만들기

### 📁 프로젝트 구조

```
Server_API/
├── main.cpp                    # 서버 시작점
├── net/
│   ├── HttpServer.h/cpp       # HTTP 서버 클래스
│   └── HttpSession.h/cpp      # HTTP 세션 처리
├── config/
│   └── ApiConfig.h/cpp        # API 서버 설정
└── utils/ (채팅서버 공유)
```

### 🔧 핵심 기능

- **기본 HTTP 서버** 실행 (포트 8081)
- **GET /health** - 서버 상태 확인
- **GET /api/test** - 간단한 JSON 응답
- **멀티스레드** 처리

### 🧪 테스트

```bash
curl http://localhost:8081/health
# 응답: {"status": "ok", "server": "API Server"}

curl http://localhost:8081/api/test  
# 응답: {"message": "Hello API Server!"}
```

### 📝 브랜치 전략

```bash
git checkout -b feat/250603_phase1_http_basic
```

---

## 🎯 Phase 2: 라우팅 시스템 (1주)

### 🎯 **목표**: REST API 엔드포인트 구조 만들기

### 📁 추가 구조

```
Server_API/
├── routes/
│   ├── ApiRouter.h/cpp        # 라우터 메인
│   ├── HealthRoutes.h/cpp     # 헬스체크 라우트
│   └── TestRoutes.h/cpp       # 테스트 라우트
├── controllers/
│   ├── HealthController.h/cpp # 헬스체크 컨트롤러
│   └── TestController.h/cpp   # 테스트 컨트롤러
└── models/
    └── ApiResponse.h          # 응답 모델
```

### 🔧 핵심 기능

- **라우팅 시스템** (URL → Controller 매핑)
- **HTTP 메소드** 처리 (GET, POST, PUT, DELETE)
- **JSON 응답** 표준화
- **에러 핸들링** 기초

### 🧪 테스트

```bash
GET  /api/health              # 서버 상태
GET  /api/test                # 테스트 메시지
POST /api/test                # POST 테스트
GET  /api/info                # 서버 정보
```

### 📝 브랜치 전략

```bash
git checkout -b feat/250610_phase2_routing_system
```

---

## 🎯 Phase 3: 사용자 인증 시스템 (1-2주)

### 🎯 **목표**: JWT 기반 로그인/회원가입 구현

### 📁 추가 구조

```
Server_API/
├── controllers/
│   ├── AuthController.h/cpp   # 인증 컨트롤러
│   └── UserController.h/cpp   # 사용자 컨트롤러
├── services/
│   ├── AuthService.h/cpp      # 인증 비즈니스 로직
│   └── UserService.h/cpp      # 사용자 비즈니스 로직
├── middleware/
│   └── AuthMiddleware.h/cpp   # JWT 토큰 검증
├── models/
│   ├── User.h                 # 사용자 모델
│   ├── LoginRequest.h         # 로그인 요청
│   └── LoginResponse.h        # 로그인 응답
└── security/
    └── JwtManager.h/cpp       # JWT 토큰 관리
```

### 🔧 핵심 기능

- **회원가입** API
- **로그인** API
- **JWT 토큰** 발급/검증
- **인증 미들웨어** (보호된 엔드포인트)
- **채팅서버 DB 연동** (Users 테이블 활용)

### 🧪 테스트

```bash
POST /api/auth/register        # 회원가입
POST /api/auth/login           # 로그인
GET  /api/auth/profile         # 프로필 조회 (JWT 필요)
PUT  /api/auth/profile         # 프로필 수정 (JWT 필요)
```

### 📝 브랜치 전략

```bash
git checkout -b feat/250617_phase3_auth_system
```

---

## 🎯 Phase 4: 게임 데이터 API (1-2주)

### 🎯 **목표**: 게임 통계, 매치 데이터 관리

### 📁 추가 구조

```
Server_API/
├── controllers/
│   ├── StatsController.h/cpp  # 통계 컨트롤러
│   ├── MatchController.h/cpp  # 매치 컨트롤러
│   └── RankController.h/cpp   # 랭킹 컨트롤러
├── services/
│   ├── StatsService.h/cpp     # 통계 서비스
│   └── MatchService.h/cpp     # 매치 서비스
├── models/
│   ├── GameStats.h            # 게임 통계
│   ├── Match.h                # 매치 정보
│   └── Ranking.h              # 랭킹 정보
└── db/
    └── GameRepository.h/cpp   # 게임 데이터 저장소
```

### 🔧 핵심 기능

- **개인 통계** API (K/D, 승률 등)
- **매치 히스토리** API
- **랭킹/리더보드** API
- **게임 결과 저장** API
- **DB 스키마 확장** (Scores, Rankings 테이블 활용)

### 🧪 테스트

```bash
GET  /api/stats/user/{id}      # 개인 통계
GET  /api/matches/user/{id}    # 매치 히스토리
GET  /api/rankings/leaderboard # 리더보드
POST /api/matches/result       # 게임 결과 저장
```

### 📝 브랜치 전략

```bash
git checkout -b feat/250624_phase4_game_data
```

---

## 🎯 Phase 5: 매치메이킹 시스템 (1-2주)

### 🎯 **목표**: 게임 매치 생성/참가 시스템

### 📁 추가 구조

```
Server_API/
├── controllers/
│   └── MatchmakingController.h/cpp
├── services/
│   ├── MatchmakingService.h/cpp
│   └── QueueService.h/cpp
├── models/
│   ├── MatchmakingRequest.h
│   ├── GameSession.h
│   └── Queue.h
└── matchmaking/
    ├── QueueManager.h/cpp     # 대기열 관리
    └── MatchAlgorithm.h/cpp   # 매칭 알고리즘
```

### 🔧 핵심 기능

- **매치메이킹 신청/취소** API
- **대기열 상태** 조회
- **게임 세션 생성**
- **MMR 기반 매칭** 알고리즘
- **실시간 대기열** 관리

### 🧪 테스트

```bash
POST /api/matchmaking/queue    # 대기열 등록
DELETE /api/matchmaking/queue  # 대기열 탈퇴  
GET  /api/matchmaking/status   # 대기 상태
GET  /api/sessions/active      # 활성 게임 세션
```

### 📝 브랜치 전략

```bash
git checkout -b feat/250701_phase5_matchmaking
```

---

## 🔄 통합 단계

### 🛠️ Phase 6: 리팩토링 & 최적화 (1주)

```bash
git checkout -b feat/250708_refactoring
```

- **코드 정리** 및 **성능 최적화**
- **에러 처리** 강화
- **로깅 시스템** 완성
- **설정 파일** 통합
- **단위 테스트** 추가

### 🔗 Phase 7: 채팅서버 연동 (1주)

```bash
git checkout -b feat/250715_chat_integration
```

- **공통 DB** 스키마 통합
- **사용자 세션** 동기화
- **JWT 토큰** 공유
- **API ↔ Chat** 통신 테스트

### 🎯 Phase 8: 통합 테스트 (1주)

```bash
git checkout server/api
git merge feat/250715_chat_integration
```

- **전체 시스템** 테스트
- **부하 테스트**
- **문서화** 완성

---

## 📅 예상 일정 (총 8-10주)

|Phase|기간|브랜치|핵심 기능|
|---|---|---|---|
|1|1주|`feat/250603_phase1_http_basic`|HTTP 서버 기초|
|2|1주|`feat/250610_phase2_routing`|라우팅 시스템|
|3|1-2주|`feat/250617_phase3_auth`|JWT 인증|
|4|1-2주|`feat/250624_phase4_game_data`|게임 데이터 API|
|5|1-2주|`feat/250701_phase5_matchmaking`|매치메이킹|
|6|1주|`feat/250708_refactoring`|리팩토링|
|7|1주|`feat/250715_chat_integration`|채팅서버 연동|
|8|1주|`server/api` merge|통합 테스트|

---

## 🎯 각 Phase별 완료 기준

### ✅ Phase 완료 체크리스트

- [ ] 기능 구현 완료
- [ ] 로컬 테스트 통과
- [ ] 코드 리뷰 (본인)
- [ ] 문서 업데이트
- [ ] 다음 Phase 브랜치 생성

### 🔄 Phase 간 이동

```bash
# Phase N 완료 후
git add .
git commit -m "Phase N 완료: [기능 설명]"
git push origin feat/YYMMDD_phaseN_기능명

# 다음 Phase 시작  
git checkout server/api
git merge feat/YYMMDD_phaseN_기능명
git checkout -b feat/YYMMDD_phaseN+1_기능명
```

이렇게 하면 **체계적이고 안전한 학습 중심 개발**이 가능합니다! 🚀