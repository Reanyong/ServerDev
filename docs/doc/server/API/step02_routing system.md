# Step02: REST API 라우팅 시스템 및 MVC 아키텍처

## 🎯 목표
Boost.Beast 기반 HTTP 서버에 체계적인 라우팅 시스템을 구축하고 MVC 패턴을 도입합니다.  
컨트롤러 기반의 요청 처리 구조를 통해 확장 가능하고 유지보수성이 높은 REST API 서버를 구현하는 데 목적이 있습니다.

---

## ⚙️ 주요 기능

- **체계적인 라우팅 시스템** (URL → Controller 매핑)
- **MVC 패턴** 기반 아키텍처 구조
- **HTTP 메소드별 처리** (GET, POST, PUT, DELETE)
- **표준화된 JSON API 응답** 구조
- **컨트롤러 기반 요청 처리** (HealthController, TestController)
- **HTTP 메소드 추상화** (타입 안전성 확보)
- **CORS 헤더 지원** (Cross-Origin 요청 허용)
- **체계적인 에러 핸들링** (HTTP 상태 코드별 응답)
- **멀티스레드 비동기 처리** (세션별 독립적 처리)
- **확장 가능한 구조** (새로운 컨트롤러 쉽게 추가 가능)

---

## 🌐 API 엔드포인트

### `GET /health`
서버 상태 및 헬스체크 정보
```json
{
  "status": 200,
  "data": {
    "server": "API Server",
    "version": "1.0.0",
    "status": "healthy",
    "uptime": "계산 필요",
    "thread_count": 16
  },
  "message": "Server is running normally",
  "timestamp": 1234567890,
  "path": "/health",
  "method": "GET"
}
```

### `GET /api/test`
GET 메소드 테스트용 엔드포인트
```json
{
  "status": 200,
  "data": {
    "message": "Hello from TestController!",
    "path": "/api/test",
    "query": "",
    "method": "GET"
  },
  "message": "GET request processed successfully",
  "timestamp": 1234567890,
  "path": "/api/test",
  "method": "GET"
}
```

### `POST /api/test`
POST 메소드 테스트용 엔드포인트
```json
{
  "status": 200,
  "data": {
    "message": "POST request received",
    "path": "/api/test",
    "body": "request body content",
    "method": "POST"
  },
  "message": "POST request processed successfully",
  "timestamp": 1234567890,
  "path": "/api/test",
  "method": "POST"
}
```

### `PUT /api/test`
PUT 메소드 테스트용 엔드포인트
```json
{
  "status": 200,
  "data": {
    "message": "PUT request received",
    "path": "/api/test",
    "body": "request body content",
    "method": "PUT"
  },
  "message": "PUT request processed successfully",
  "timestamp": 1234567890
}
```

### `DELETE /api/test`
DELETE 메소드 테스트용 엔드포인트
```json
{
  "status": 200,
  "data": {
    "message": "DELETE request received",
    "path": "/api/test",
    "query": "",
    "method": "DELETE"
  },
  "message": "DELETE request processed successfully",
  "timestamp": 1234567890
}
```

### `404 Not Found`
존재하지 않는 경로 접근 시
```json
{
  "status": 404,
  "error": "Not Found",
  "message": "The requested endpoint '/invalid/path' was not found",
  "path": "/invalid/path",
  "timestamp": 1234567890
}
```

---

## 🧪 테스트 방법

1. Visual Studio 2022에서 `Server_API` 프로젝트 실행
2. 브라우저나 Postman에서 다음 URL들 테스트:
   ```
   http://localhost:8081/health
   http://localhost:8081/api/test
   ```
3. 다양한 HTTP 메소드로 테스트:
   ```bash
   # GET 테스트
   curl http://localhost:8081/api/test
   
   # POST 테스트
   curl -X POST http://localhost:8081/api/test -d "test data"
   
   # PUT 테스트  
   curl -X PUT http://localhost:8081/api/test -d "update data"
   
   # DELETE 테스트
   curl -X DELETE http://localhost:8081/api/test
   
   # 404 테스트
   curl http://localhost:8081/invalid
   ```
4. 콘솔에서 요청 로그 확인
5. Ctrl+C로 서버 정상 종료 확인

## 💻 구현 내용

### 핵심 아키텍처 구조
```
Server_API/
├── main.cpp                    # 서버 진입점 및 신호 처리
├── config/
│   └── ApiConfig.h/cpp        # 서버 설정 관리
├── net/
│   ├── HttpServer.h/cpp       # HTTP 서버 메인 클래스
│   └── HttpSession.h/cpp      # 개별 HTTP 세션 처리
├── routes/
│   └── ApiRouter.h/cpp        # URL → Controller 매핑 라우터
├── controllers/
│   ├── BaseController.h/cpp   # 컨트롤러 기본 클래스
│   ├── HealthController.cpp   # 헬스체크 컨트롤러
│   └── TestController.cpp     # 테스트 컨트롤러
└── models/
    ├── ApiResponse.h/cpp      # JSON 응답 모델
    └── HttpMethod.h           # HTTP 메소드 추상화
```

### MVC 패턴 구현
- **Model**: `ApiResponse`, `HttpMethod` - 데이터 모델
- **View**: JSON 응답 - 클라이언트에게 전달되는 데이터 표현
- **Controller**: `BaseController`, `HealthController`, `TestController` - 비즈니스 로직 처리

### 라우팅 시스템
```cpp
class ApiRouter {
private:
    std::unordered_map<std::string, std::shared_ptr<BaseController>> routes_;
    
public:
    void registerRoute(const std::string& path, std::shared_ptr<BaseController> controller);
    ApiResponse handleRequest(HttpMethod method, const std::string& path, ...);
};
```

### HTTP 메소드 추상화
```cpp
enum class HttpMethod {
    Get, Post, Put, Delete, Patch, Options, Head, Unknown
};

class HttpMethodUtils {
public:
    static HttpMethod fromBeastMethod(http::verb method);
    static std::string toString(HttpMethod method);
    static http::verb toBeastMethod(HttpMethod method);
};
```

### 표준화된 API 응답
```cpp
class ApiResponse {
private:
    http::status status_;
    nlohmann::json response_data_;
    
public:
    ApiResponse& setData(const nlohmann::json& data);
    ApiResponse& setMessage(const std::string& message);
    ApiResponse& setError(const std::string& error);
    ApiResponse& setTimestamp(std::time_t timestamp = std::time(nullptr));
    http::response<http::string_body> toHttpResponse(unsigned version) const;
};
```

### 주요 기술 스택
- **C++20** (모던 C++ 기능 활용)
- **Boost.Beast** (HTTP 서버 구현)
- **Boost.Asio** (비동기 네트워킹)
- **nlohmann/json** (JSON 처리)
- **스마트 포인터** (메모리 안전성)
- **RAII 패턴** (리소스 관리)

### 설정 및 구성
- **포트**: 8081 (채팅서버 8080과 구분)
- **주소**: 0.0.0.0 (모든 인터페이스)
- **스레드**: 하드웨어 스레드 수 자동 감지
- **문자셋**: UTF-8
- **CORS**: 모든 Origin 허용

---

## 📌 참고 사항

- Boost 1.82 이상 권장
- vcpkg 패키지 매니저로 의존성 관리
- Server_Chat 프로젝트의 `ConsoleHelper` 유틸리티 공유 사용
- Visual Studio 2022 (v143 툴셋) 필요
- C++20 표준 사용
- 새로운 컨트롤러 추가 시 `ApiRouter` 생성자에서 라우트 등록 필요
- 모든 API 응답은 표준화된 JSON 구조 사용
- CORS 헤더가 자동으로 추가되어 웹 브라우저에서 직접 호출 가능

---

## 🔮 다음 단계 (Step03)

- **JWT 기반 인증 시스템** 구현
- **미들웨어 패턴** 도입 (인증 미들웨어)
- **데이터베이스 연동** (PostgreSQL)
- **사용자 관리 API** (회원가입, 로그인)
- **보안 강화** (입력 검증, SQL 인젝션 방지)

---

> 작성자: [rNyong]  
> 작성일: 2025-01-27 