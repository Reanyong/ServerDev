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

## 📅 2025-03-31 🧪 한글 UTF-8 출력 깨짐 문제

- **문제**  
    Windows 콘솔에서 WebSocket 수신 메시지 중 한글이 깨진채로 "궔뤣줽..." 출력됨
    
- **원인 분석**  
    Windows 콘솔의 기본 코드페이지가 CP949(Korean ANSI)로 설정되어 있어 UTF-8 문자열을 제대로 출력하지 못함. 또한, `std::cout`은 wchar를 직접 다루기 어려워 한글 출력 시 깨짐 발생.
    
- **시도한 해결법**
    
- **최종 해결 방법**  
    위 3가지 모두 적용하여 해결됨. WebSocket으로 받은 한글 메시지가 콘솔에 정상적으로 출력됨.
    
- **관련 커밋**  
    `feat: 한글 지원 기능 추가 echo and add HTML client test page`
    
- **느낀 점**  
    콘솔 출력도 로컬 테스트에서 굉장히 중요하다. 특히 다국어 지원을 고려할 경우, 출력 코드페이지와 출력 스트림 타입(`wcout`)을 명확히 이해해야 함.
    UTF-8은 진짜....너무하다...... 미국인 하고 싶다 그냥...

---

# 📅 2025-04-03 🧪 Visual Studio 소스 내 한글 문자열 오류

- **문제**  
    Visual Studio 2022에서 Boost.Beast 코드 내 람다 함수에 한글 문자열이 포함된 경우  
    "상수에 줄 바꿈 문자가 있습니다", "일치하는 토큰을 찾을 수 없습니다" 컴파일 오류 발생.

- **원인 분석**  
    Visual Studio의 C++ 컴파일러가 특정 인코딩(EUC-KR/CP949)으로 저장된 파일의 한글 문자열을 처리하는 과정에서 문제 발생.  
    특히 람다 함수 내에서 `L"[Server] 새 클라이언트 연결 수락됨"` 같은 한글 문자열이 포함된 경우  
    컴파일러가 줄바꿈으로 잘못 인식함.

- **시도한 해결법**
	- [x]  코드 재구성 및 줄바꿈 제거
	- [x]  람다 함수 분리 및 간소화
	- [x]  Visual Studio 재시작
	- [x]  프로젝트 설정에서 문자 집합 옵션 변경
	- [x]  새 파일 생성 후 코드 복사
	- [x]  다른 인코딩으로 저장 시도 (UTF-8 with BOM)

- **최종 해결 방법**  
    파일을 **유니코드(서명 없는 UTF-8)** 인코딩으로 저장하여 문제 해결.  
    Visual Studio의 **파일 저장 옵션**에서 인코딩 설정을 명시적으로 변경함.  
    추가로 프로젝트 전체에 일관된 인코딩을 적용하기 위해 `.editorconfig` 활용 예정.

- **관련 커밋**  
    `fix: Server Korean Encoding`

- **느낀 점**  
    한글 인코딩은 정말 복잡한 문제였다.  
    Visual Studio에서 C++ 코드의 인코딩 처리 방식이 상당히 까다로웠다.  
    이로 인해 `.editorconfig` 파일의 중요성을 알게 되었고,  
    협업 시 이런 기본 설정을 **명확히 정의**하는 것이 얼마나 중요한지 체감했다.  
    인코딩 변환에 의존하기보다는 **명시적으로 UTF-8을 지정하는 습관**을 들이기로 했다.

---

## 📅 2025-04-05 🧪 멀티스레드 WebSocket 서버 구현 중 발생한 주요 이슈 정리

---

### 🧵 스레드 생성 및 실행 문제

- **문제**  
    `threads_.size()`를 기반으로 반복문을 실행했으나, 벡터가 아직 비어있어 스레드가 생성되지 않고 서버가 즉시 종료됨
    
- **원인 분석**  
    `threads_`는 아직 비어있는 상태에서 `.size()` 상태로 루프를 돌리면 아무 작업도 수행되지 않음
    
- **시도한 해결법**
    
    -  `threads_.size()` → `num_threads` 변수 사용
    -  `thread_count`를 명시적으로 계산 후 루프 사용
        
- **최종 해결 방법**  
    `int num_threads = max(thread_count, 1);` 이후 `for (int i = 0; i < num_threads; ++i)`로 수정    

---

### 🔢 hash[thread::id](thread::id)()의 컴파일러 호환성 문제

- **문제**  
    `thread::id().hash()` 형태가 Visual Studio에서 컴파일 오류 발생
    
- **원인 분석**  
    표준에서는 `std::hash<thread::id>` 특수화를 이용해야 하며, 직접 `.hash()` 멤버는 존재하지 않음
    
- **시도한 해결법**
    
    -  `hash<thread::id>{}(this_thread::get_id())` 형식으로 변경
        
- **최종 해결 방법**  
    `std::to_wstring(hash<thread::id>{}(std::this_thread::get_id()))` 형태로 해결
    

---

### 🔐 동시 쓰기 작업으로 인한 race condition

- **문제**  
    WebSocket에 여러 스레드가 동시에 쓰기 시도 시 충돌 가능성 (race condition)
    
- **원인 분석**  
    여러 클라이언트 메시지를 동시에 처리하려 할 때 `async_write`의 내부 리소스 접근이 충돌
    
- **시도한 해결법**
    
    -  `write_mutex_` 멤버 변수 추가
        
    -  `on_read()` 내 쓰기 전에 `lock_guard<mutex>` 사용
        
- **최종 해결 방법**  
    쓰기 동기화 적용 후 충돌 없이 echo 처리 성공
    

---

### 🖨️ 콘솔 출력 중첩 문제

- **문제**  
    여러 스레드에서 동시에 콘솔에 출력하면서 메시지 겹침, 출력 깨짐 발생
    
- **원인 분석**  
    `std::wcout` 또는 `WriteConsoleW` 호출이 스레드 세이프하지 않음
    
- **시도한 해결법**
    
    -  `console_mutex` 생성
        
    -  모든 콘솔 출력 함수에 `lock_guard` 적용
        
- **최종 해결 방법**  
    동기화 적용 후 콘솔 메시지 정렬 정상화됨
    

---

### ⚙️ io_context 크기 설정 이슈

- **문제**  
    `io_context` 생성 시 concurrency hint 미지정 시 성능 저하 발생 우려
    
- **원인 분석**  
    멀티스레드에서 각 스레드가 별도로 작업할 수 있도록 `io_context`에 적절한 concurrency 수 전달 필요
    
- **시도한 해결법**
    
    -  `thread::hardware_concurrency()` 사용해 동시성 계산
        
    -  `net::io_context ioc{thread_count}` 형태로 변경
        
- **최종 해결 방법**  
    `thread_count` 기반으로 `io_context` 생성하여 성능 문제 해결
    

---

- **관련 커밋**  
    `feat: multithread async websocket echo server`

- **느낀 점**  
    네트워크 서버에서 멀티스레드 적용 시 비동기만큼이나 자원 동기화 및 콘솔 로깅 안정성도 중요함을 체감함. 특히 Boost 기반 비동기 설계에 있어 스레드 안전성 확보는 핵심 고려 사항. 추가적으로 스레드 관리는 진짜 어디서나 어려운거 같다.
    

---