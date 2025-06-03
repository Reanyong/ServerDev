# Step01: 기본 HTTP REST API 서버

## 🎯 목표
Boost.Beast를 사용하여 기본적인 HTTP REST API 서버를 구현합니다.  
JSON 응답을 제공하는 간단한 엔드포인트들을 통해 HTTP 서버의 동작 원리와 RESTful API 구조를 학습하는 데 목적이 있습니다.

---

## ⚙️ 주요 기능

- HTTP 요청/응답 처리 (Boost.Beast 기반)
- 멀티스레드 비동기 처리 (하드웨어 스레드 수 자동 감지)
- JSON 형태의 API 응답 제공
- CORS 헤더 지원 (Cross-Origin 요청 허용)
- 기본 라우팅 시스템 (간단한 URL → 핸들러 매핑)
- Windows 콘솔 시그널 처리 (Ctrl+C 종료)
- 컬러 콘솔 로깅 (Server_Chat의 ConsoleHelper 활용)

---

## 🌐 API 엔드포인트

### `GET /health`
서버 상태 확인용 헬스체크 엔드포인트
```json
{
  "status": "ok",
  "server": "API Server", 
  "version": "1.0.0"
}
```

### `GET /api/test`
기본 테스트용 엔드포인트
```json
{
  "message": "Hello API Server!",
  "timestamp": 1234567890,
  "method": "GET"
}
```

### `404 Not Found`
존재하지 않는 경로 접근 시
```json
{
  "error": "Not Found",
  "path": "/invalid/path"
}
```

---

## 🧪 테스트 방법

1. Visual Studio 2022에서 `Server_API` 프로젝트 실행
2. 브라우저나 Postman에서 다음 URL들 테스트:
   ```
   http://localhost:8081/health
   http://localhost:8081/api/test
   http://localhost:8081/invalid (404 테스트)
   ```
3. 콘솔에서 요청 로그 확인
4. Ctrl+C로 서버 정상 종료 확인

### curl 테스트 예시
```bash
# 헬스체크
curl http://localhost:8081/health

# 테스트 API
curl http://localhost:8081/api/test

# 404 테스트
curl http://localhost:8081/notfound
```

---

## 💻 구현 내용

### 핵심 클래스 구조
- **`ApiConfig`**: 서버 설정 관리 (주소, 포트, 스레드 수)
- **`HttpServer`**: HTTP 서버 메인 클래스 (Boost.Asio 기반)
- **`HttpSession`**: 개별 HTTP 요청/응답 처리 세션
- **`main.cpp`**: 서버 진입점 및 시그널 처리

### 주요 기술 스택
- **C++20** (모던 C++ 기능 활용)
- **Boost.Beast** (HTTP 서버 구현)
- **Boost.Asio** (비동기 네트워킹)
- **nlohmann/json** (JSON 처리)
- **멀티스레딩** (IO Context 스레드 풀)

### 설정
- **포트**: 8081 (채팅서버 8080과 구분)
- **주소**: 0.0.0.0 (모든 인터페이스)
- **스레드**: 하드웨어 스레드 수 자동 감지
- **문자셋**: Unicode

---

## 📌 참고 사항

- Boost 1.82 이상 권장
- vcpkg 패키지 매니저로 의존성 관리
- Server_Chat 프로젝트의 `ConsoleHelper` 유틸리티 공유 사용
- Visual Studio 2022 (v143 툴셋) 필요
- C++20 표준 사용
- 현재는 간단한 라우팅만 구현 (향후 확장 예정)
- 인증, 데이터베이스 연동 등은 다음 단계에서 구현

---

> 작성자: [rNyong]  
> 작성일: 2025-01-27 