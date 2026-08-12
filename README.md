# 2023 어드벤처디자인 — RC카를 이용한 C-ITS 구현

**한국어** · [日本語](README.ja.md) · [English](README.en.md)

동국대학교 전자전기공학부 **2023학년도 2학기 어드벤처디자인** 기말 프로젝트.
Flask 신호 서버 · WiFi 신호등 노드 · nRF24L01 무선 링크 · 자율 주행 RC카를 엮어 **C-ITS(Cooperative Intelligent Transport System, 차세대 지능형 교통체계)** 의 V2I(차량-인프라) 협력 주행을 소형 하드웨어로 재현한 작품입니다.

## 소개

실제 C-ITS 에서는 도로 인프라(신호등)가 자신의 신호 위상과 잔여시간을 무선으로 방송하고, 차량은 이를 수신해 정지선 앞에서 스스로 감속·정지·출발을 판단합니다. 본 프로젝트는 이 흐름을 4개의 노드로 축소 구현했습니다.

```
[Flask 신호 서버]  --HTTP/JSON-->  [신호등 노드(UNO R4 WiFi)]  --nRF24L01-->  [RF 수신 릴레이]  --Serial-->  [RC카]
   신호 위상·                        LED·7세그·서보 차단기                       무선 신호 중계              정지선 IR + 신호 판단
   잔여시간 생성                      + 무선 방송                                                          → 자율 정지/출발
```

- **Flask 서버**가 신호 위상(적/황/녹)과 카운트다운을 3초 주기로 갱신하며 JSON 으로 제공
- **신호등 노드**가 WiFi 로 서버를 폴링해 실제 LED·7-segment·서보 차단기를 구동하고, 현재 신호를 nRF24L01 로 방송
- **RF 수신 릴레이**가 무선 신호를 받아 Serial 로 RC카에 전달
- **RC카**가 IR 정지선 센서와 수신 신호를 종합해 적/황색이면 정지선에서 멈추고, 녹색이면 통과 (블루투스로 수동 조종도 가능)

## ✨ 주요 기능 / 동작

| 노드 | 파일 | 역할 |
|------|------|------|
| 신호 서버 | `flask.py` | 신호 위상·잔여시간 상태머신, JSON API 제공 |
| 신호등 노드 | `traffic.ino` | 서버 폴링 → LED / 7-segment 카운트다운 / 서보 차단기 구동 + nRF24L01 방송 |
| RF 수신 릴레이 | `rf.ino` | nRF24L01 신호 수신 → Serial 중계 |
| RC카 | `rccar.ino` | 블루투스 조종 + IR 정지선 감지 + 신호 연동 자율 정지/출발 |

### 신호 상태머신 (`flask.py`)
- `APScheduler` 로 **3초 간격**(`interval`, `Asia/Seoul`)으로 신호 갱신
- 신호 코드: **14 = 적색(red)**, **15 = 황색(yellow)**, **16 = 녹색(green)**
- 적색이면 `time` 을 6→0 으로 카운트다운 후 녹색으로 전환, 녹색이면 5→0 카운트다운 후 황색, 황색이면 적색으로 전환
- `GET /` 요청 시 `{ id, description, signal, time }` JSON 반환 (`host=0.0.0.0`, `port=80`)

### 신호등 노드 (`traffic.ino`)
- **WiFi(WiFiS3)** 로 Flask 서버(`172.20.10.7:80`)에 HTTP GET → 응답 JSON 에서 `signal`·`time` 파싱(`ArduinoJson`)
- **신호 LED 3색**: `r=핀14`, `y=핀15`, `g=핀16`
- **7-segment(common-anode)**: 세그먼트 핀 `{2,3,4,5,6,7,8,9}`, `digits[10][8]` 패턴으로 잔여시간 카운트다운 표시
- **서보 차단기**(`핀 10`): 적색 진입 시 `write(100)`(내림), 녹색 진입 시 `write(10)`(올림)
- **nRF24L01 송신**: 현재 신호 문자(`'r'`/`'y'`/`'g'`)를 RC카 쪽으로 무선 방송

### RF 수신 릴레이 (`rf.ino`)
- nRF24L01 수신 전용 — 라디오로 받은 문자열을 그대로 `Serial.write()` 로 RC카 보드에 전달

### RC카 제어 (`rccar.ino`)
- **블루투스(SoftwareSerial)** 수동 조종: `'w'` = 전진, `'x'` = 정지
- **IR 거리 센서**(`A1`)로 정지선 감지 (아날로그값 `< 400` → 정지선 위)
- **신호 연동 자율 주행**(`motor_drive()`):
  - 평상시 도로(`adc_Ir > 400`) → 블루투스 명령대로 주행
  - 정지선 감지 + 신호 ≠ 녹색(`'g'`) → **정지**
  - 신호 = 녹색 → 통과 주행

## 🛠 기술 스택 / 하드웨어

**언어 / 프레임워크**
- Python 3 — Flask, APScheduler (신호 서버)
- C++ (Arduino) — 3개의 `.ino` 노드

**Arduino 라이브러리**
- `WiFiS3` (UNO R4 WiFi) · `ArduinoJson` · `Servo` — 신호등 노드
- `SPI` / `nRF24L01` / `RF24` (nRF24L01 무선 링크) — 신호등 노드 · RF 수신 릴레이
- `SoftwareSerial` (블루투스) — RC카

**하드웨어 (코드에서 확인된 범위)**
- MCU: 신호등 노드 = Arduino **UNO R4 WiFi**, RC카 / 수신 릴레이 = Arduino (SPI·SoftwareSerial 구동)
- nRF24L01 2.4GHz 무선 모듈 (주소 `"00001"`, `RF24_PA_MIN`)
- 신호 LED ×3, 7-segment(common-anode) ×1, 서보모터 ×1(차단기)
- DC 모터 ×2 + 모터 드라이버, IR 거리 센서, 블루투스 모듈

## 📌 핀 배치 (코드 기준)

**신호등 노드 (`traffic.ino`)**

| 핀 | 연결 | 비고 |
|----|------|------|
| 14 / 15 / 16 | 신호 LED 적 / 황 / 녹 | `r` / `y` / `g` |
| 2, 3, 4, 5, 6, 7, 8, 9 | 7-segment 세그먼트 | 잔여시간 표시 |
| 10 | 서보모터 | 차단기 (적색 내림 / 녹색 올림) |
| 17 (CE) / 18 (CSN) | nRF24L01 | 신호 송신 |

**RC카 (`rccar.ino`)**

| 핀 | 연결 | 비고 |
|----|------|------|
| A1 | IR 거리 센서 | 정지선 감지 (임계값 400) |
| 10 / 11 | 모터 A / B 속력(PWM) | `M_SPEED=120`, 균형 `A_BAL=1`, `B_BAL=0.865` |
| 12 / 13 | 모터 A / B 방향 | |
| 8 (RXD) / 7 (TXD) | 블루투스 (SoftwareSerial) | 9600bps, `'w'`/`'x'` 명령 |

**RF 수신 릴레이 (`rf.ino`)**

| 핀 | 연결 |
|----|------|
| 7 (CE) / 8 (CSN) | nRF24L01 수신 |

## 🚀 사용법

### 1. Flask 신호 서버 실행
```bash
pip install flask apscheduler
python flask.py    # 0.0.0.0:80 에서 신호 API 제공
```

### 2. 신호등 노드 (`traffic.ino`) 업로드
- Arduino IDE 에서 `WiFiS3` · `ArduinoJson` · `Servo` · `RF24` 라이브러리 설치
- 코드 상단의 WiFi SSID / PASSWORD 와 서버 IP(`server[]`)를 실제 Flask 서버 주소로 수정
  (WiFi 자격증명은 `arduino_secrets.h` 로 분리 관리 가능)
- UNO R4 WiFi 에 업로드

### 3. RF 수신 릴레이(`rf.ino`) · RC카(`rccar.ino`) 업로드
- 각각 별도 Arduino 보드에 업로드
- 수신 릴레이의 Serial 출력을 RC카 보드의 Serial 입력에 연결
- RC카는 블루투스 페어링 후 `'w'`/`'x'` 로 조종, 신호등 노드의 방송에 따라 정지선에서 자율 정지/출발

## 📁 구조

```
2023_adventure_design/
├─ flask.py       # Python Flask 신호 서버 (신호 위상·잔여시간 상태머신 + JSON API)
├─ traffic.ino    # 신호등 노드 (UNO R4 WiFi): 서버 폴링 → LED/7세그/서보 + nRF24L01 방송
├─ rf.ino         # RF 수신 릴레이 (nRF24L01 → Serial 중계)
└─ rccar.ino      # RC카 제어 (블루투스 조종 + IR 정지선 + 신호 연동 자율 주행)
```

---

> 학습/교육용 프로젝트입니다. WiFi SSID·비밀번호, 서버 IP 등은 시연 환경(개인 핫스팟) 기준으로 하드코딩되어 있으므로 실제 사용 시 반드시 본인 환경에 맞게 수정하세요.

---

## 👤 기여도 & 개발 환경

| 항목 | 내용 |
|---|---|
| **기여 비율** | **100%** (단독 개발) |
| **커밋** | 3 / 3 (본인 / 전체 사람 커밋) |
| **참여 인원** | 1명 |

<sub>기여 비율은 커밋 author 이메일 기준 집계이며 봇·자동화 커밋은 제외했습니다.</sub>
