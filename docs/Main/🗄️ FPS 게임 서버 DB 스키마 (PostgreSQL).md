본 문서는 FPS 게임 서버의 PostgreSQL 기반 데이터베이스 구조를 설명합니다.  
일부 테이블은 TimeScaleDB의 하이퍼테이블로 변환되어 시계열 데이터 저장을 효율화합니다.

---

## 📋 테이블 목록

- [Users](#users)
- [Sessions](#sessions) *(TimeScaleDB)*
- [BattleLogs](#battlelogs) *(TimeScaleDB)*
- [GameSessions](#gamesessions) *(TimeScaleDB)*
- [Scores](#scores)
- [Rankings](#rankings)
- [MMR_Calculations](#mmr_calculations)
- [Chats](#chats)
- [Messages](#messages)
- [Matchmaking](#matchmaking)
- [ServerRegions](#serverregions)

---

## 🧑‍💼 Users

```sql
CREATE TABLE Users (
  id UUID PRIMARY KEY,
  username VARCHAR(50) UNIQUE,
  email VARCHAR(100) UNIQUE,
  created_at TIMESTAMPTZ
);
```

---

## 🔐 Sessions (TimeSeries)

```sql
CREATE TABLE Sessions (
  session_id UUID PRIMARY KEY,
  user_id UUID REFERENCES Users(id),
  status VARCHAR(20),
  created_at TIMESTAMPTZ
);

-- TimeScaleDB 변환
SELECT create_hypertable('Sessions', 'created_at');
```

---

## ⚔️ BattleLogs (TimeSeries)

```sql
CREATE TABLE BattleLogs (
  id SERIAL PRIMARY KEY,
  match_id UUID,
  user_id UUID REFERENCES Users(id),
  target_id UUID REFERENCES Users(id),
  weapon VARCHAR(50),
  damage INT,
  created_at TIMESTAMPTZ
);

-- TimeScaleDB 변환
SELECT create_hypertable('BattleLogs', 'created_at');
```

---

## 🗺️ GameSessions (TimeSeries)

```sql
CREATE TABLE GameSessions (
  session_id UUID PRIMARY KEY,
  region VARCHAR(50),
  event_type VARCHAR(50),
  event_data JSONB,
  created_at TIMESTAMPTZ
);

-- TimeScaleDB 변환
SELECT create_hypertable('GameSessions', 'created_at');
```

---

## 🏆 Scores

```sql
CREATE TABLE Scores (
  id SERIAL PRIMARY KEY,
  user_id UUID REFERENCES Users(id),
  score INT,
  created_at TIMESTAMPTZ
);
```

---

## 🥇 Rankings

```sql
CREATE TABLE Rankings (
  id SERIAL PRIMARY KEY,
  user_id UUID REFERENCES Users(id),
  rank INT,
  updated_at TIMESTAMPTZ
);
```

### 🎯 트리거로 랭킹 자동 업데이트

```sql
CREATE FUNCTION update_rankings() RETURNS TRIGGER AS $$
BEGIN
  UPDATE Rankings
  SET rank = (SELECT COUNT(*) FROM Scores WHERE score > NEW.score) + 1
  WHERE user_id = NEW.user_id;
  RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trigger_update_rankings
AFTER INSERT OR UPDATE ON Scores
FOR EACH ROW
EXECUTE FUNCTION update_rankings();
```

---

## 🧠 MMR_Calculations

```sql
CREATE TABLE MMR_Calculations (
  id SERIAL PRIMARY KEY,
  user_id UUID REFERENCES Users(id),
  mmr FLOAT,
  calculated_at TIMESTAMPTZ
);
```

---

## 💬 Chats & Messages

```sql
CREATE TABLE Chats (
  id SERIAL PRIMARY KEY,
  chat_name VARCHAR(100),
  created_at TIMESTAMPTZ
);

CREATE TABLE Messages (
  id SERIAL PRIMARY KEY,
  chat_id INT REFERENCES Chats(id),
  user_id UUID REFERENCES Users(id),
  message TEXT,
  created_at TIMESTAMPTZ
);
```

---

## 🎮 Matchmaking

```sql
CREATE TABLE Matchmaking (
  id SERIAL PRIMARY KEY,
  user_id UUID REFERENCES Users(id),
  status VARCHAR(20) DEFAULT 'waiting',
  created_at TIMESTAMPTZ
);
```

---

## 🌍 ServerRegions

```sql
CREATE TABLE ServerRegions (
  id SERIAL PRIMARY KEY,
  region VARCHAR(50),
  active_players INT,
  max_capacity INT
);
```

---

## 📊 UserStats View

```sql
CREATE VIEW UserStats AS
SELECT u.id, u.username, s.score, r.rank, m.mmr
FROM Users u
LEFT JOIN Scores s ON u.id = s.user_id
LEFT JOIN Rankings r ON u.id = r.user_id
LEFT JOIN MMR_Calculations m ON u.id = m.user_id;
```

> 📌 *뷰 및 트리거는 dbDiagram.io에서 지원되지 않으므로 SQL 스크립트로 별도 관리합니다.*