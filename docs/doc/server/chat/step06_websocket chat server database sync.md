## 🎯 목표

- PostgreSQL 데이터베이스와 채팅 서버 연동
- 채팅 메시지 영구 저장 기능 구현
- 사용자 세션 관리 시스템 구축
- 웹 기반 테스트 클라이언트 개발

## 🚀 주요기능

#### 1. 데이터베이스 연동

- `libpqxx`를 사용한 PostgreSQL 연결
- `DatabaseManager` 싱글톤 패턴 구현
- 트랜잭션 기반 쿼리 실행

#### 2. 메시지 저장

- 모든 채팅 메시지 DB 저장
- 사용자별 메시지 추적
- 시스템 메시지 구분 저장

#### 3. 사용자 관리

- 사용자 등록 및 세션 생성
- UUID 기반 사용자 식별
- 닉네임 변경 기록

#### 4. 웹 클라이언트

- 실시간 WebSocket 통신
- 자동 서버 감지 기능
- 메시지 타입별 UI 구분
## 💡 학습내용

#### 1. 전역 객체 초기화 순서

- 전역 객체는 `main()` 함수 실행 전에 생성
- 의존성이 있는 경우 별도 `initialize()` 함수 필요
- 싱글톤 패턴으로 순서 제어 가능

#### 2. 시간대 처리

- **DB 저장**: 항상 UTC 사용
- **클라이언트 표시**: 로컬 시간으로 변환
- `TIMESTAMPTZ` 타입 활용

#### 3. WebSocket과 HTTP 통합

- 같은 포트에서 프로토콜 구분 가능
- 별도 포트 사용이 더 간단한 경우 많음
- 클라이언트 자동 연결 구현 방법

#### 4. 채팅 시스템 확장 전략

- **소규모**: PostgreSQL 단독
- **중규모**: Redis 캐싱 추가
- **대규모**: TimeScaleDB, 샤딩

## 🐛 해결된 이슈

#### 1. DatabaseManager 초기화 오류

**문제**: `DatabaseManager가 초기화되지 않았습니다` 오류

**원인**: 전역 ChatRoom 객체가 main() 전에 생성

**해결**:

cpp

```cpp
// ChatRoom에 별도 초기화 함수 추가
void ChatRoom::initialize() {
    // DB 접근 코드를 여기로 이동
}

// main에서 호출
g_chatRoom.initialize();
```

#### 2. Messages 테이블 INSERT 실패

**문제**: `null value in column "email"` 오류

**원인**: DB 스키마와 코드 불일치

**해결**:

- email 컬럼을 nullable로 변경
- 또는 더미 이메일 값 제공

#### 3. user_id NULL 문제

**문제**: 메시지의 user_id가 기록되지 않음

**원인**: Session::getDbUserId() 메서드 누락

**해결**:

cpp

```cpp
public:
    const string& getDbUserId() const {
        return db_user_id_;
    }
```

#### 4. 채팅방 ID 외래키 오류

**문제**: `Key (chat_id)=(1) is not present in table "chats"`

**원인**: 채팅방이 미리 생성되지 않음

**해결**: 서버 시작 시 기본 채팅방 생성 보장

#### 5. 네트워크 접속 제한

**문제**: 같은 네트워크 내 다른 PC에서 접속 불가

**원인**: Windows 방화벽 차단

**해결**: 방화벽 규칙 추가 또는 일시 해제