> 이 문서는 개발 중 발생한 이슈와 해결 과정을 기록한 문서입니다.  
> 날짜별로 정리하며, 문제 분석 및 느낀 점까지 작성합니다.

---

## 📅 2025-03-30 🧪 pqxx 종료 시 디버그 예외 발생 (_Rootnode nullptr)

- **문제**  
  Visual Studio 디버그 모드에서 `pqxx::connection::close()` 실행 시 `_Rootnode == nullptr` 예외 발생

- **원인 분석**  
  `libpqxx` 내부에서 `m_receivers.clear()` 수행 중, 디버그 STL 검사 기능이 잘못된 접근을 예외로 검출

- **시도한 해결법**  
  - [x] `txn.commit()` 위치 조정
  - [x] 트랜잭션 블록 내에서만 `pqxx::result` 사용
  - [x] `conn.close()` 호출 생략하여 예외 회피 (우회책)
  - [x] `optional<pqxx::result>` 사용 → 동일 예외 발생
  - [x] 결과를 `vector<tuple<...>>`에 복사 → 동일
  - [x] `/D_ITERATOR_DEBUG_LEVEL=0` 설정으로 STL 디버깅 무력화
  - [x] `libpqxx` 소스 코드에서 `m_receivers.clear()` 직접 확인
  - [x] `.c_str()` 호출 이후 바로 접근 제거

- **최종 해결 방법**  
  아직 미정. 릴리즈 모드에서는 발생하지 않음

- **관련 커밋**  
  `PostgreSQL 연결 테스트 코드 추가 릴리즈 모드 정상 작동, 디버그 모드 m_receivers.clear() 함수 작동 실패 상태`

- **느낀 점**  
  외부 라이브러리 사용 시 디버그 모드에서 발생하는 STL 관련 예외는 개발 환경 설정과도 밀접함을 느낌.  
  디버깅 환경에서의 예외 처리를 위한 사전 설정 중요성을 체감함.

---
