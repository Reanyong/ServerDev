# 🌿 Git 브랜치 네이밍 규칙

이 문서는 본 프로젝트의 Git 브랜치 관리 전략과 네이밍 규칙을 정의합니다.  
브랜치는 **기능 단위로 세분화하여 개발 → 통합 → 테스트 → 배포**의 흐름을 갖습니다.

---

## 📁 브랜치 구조

```text
main (또는 master)           # 제품 릴리즈용 안정 브랜치
│
├── dev-main                # 개발 통합 브랜치
│   ├── server              # 서버 모듈별 개발
│   │   ├── chat            # 채팅 기능
│   │   │   └── feat/YYMMDD_chat_proto
│   │   ├── game            # 게임 세션 관련
│   │   │   └── feat/YYMMDD_game_udp_basic
│   │   └── api             # API 서버
│   │       └── feat/YYMMDD_api_userlogin
│   ├── infra               # AWS, Docker, 배포 관련
│   └── test                # 실험 및 테스트 전용 브랜치
│       └── test/postgres_connection
```

---

## 🧩 브랜치 네이밍 규칙

| 타입       | 접두어       | 설명                         | 예시                              |
|------------|--------------|------------------------------|-----------------------------------|
| 기능 개발  | `feat/`      | 새로운 기능 개발             | `feat/250330_chat_proto`          |
| 버그 수정  | `fix/`       | 버그 핫픽스                  | `fix/connection_crash_on_exit`    |
| 리팩터링   | `refactor/`  | 코드 구조 개선               | `refactor/session_class_split`   |
| 문서 작업  | `docs/`      | 문서 추가 및 수정            | `docs/README_init`               |
| 테스트     | `test/`      | 실험적인 테스트 코드         | `test/postgres_connection`       |
| 스타일 수정| `style/`     | 코드 포맷팅, 주석 등         | `style/remove_unused_includes`   |
| 배포 관련  | `release/`   | 릴리즈 준비                  | `release/v1.0.0`                 |

---

## ✅ 규칙 요약

- `접두어/날짜_기능명` 또는 `접두어/설명` 형식을 따릅니다.
- **날짜는 선택사항**이나, 이슈 구분에 유용하므로 `YYMMDD` 형식 권장
- 브랜치는 명확한 목적에 따라 분리하고, 기능 단위로 커밋/PR 관리합니다.

---

## 📌 참고

- 브랜치는 가능한 한 작게 쪼개고 빠르게 병합합니다.
- 테스트/실험 브랜치는 `test/` 아래로, 문서는 `docs/`로 분리 관리합니다.

