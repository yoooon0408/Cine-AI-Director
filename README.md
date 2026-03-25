# Cine AI Director

> AI가 영화 감독처럼 실시간으로 최적의 카메라 샷을 선택하는 Unreal Engine 5 플러그인

영화 제작 환경에서 씬의 분위기와 상황을 분석해 카메라 앵글을 자동으로 결정합니다.
Claude AI API를 통해 씬마다 적절한 샷을 선택하고, 이 데이터로 자체 강화학습 모델을 훈련해 점점 더 정교한 연출이 가능해집니다.

---

## 작동 방식

```
씬 상황 파악          AI 샷 결정              카메라 이동
─────────────        ─────────────          ─────────────
캐릭터 위치 추적  →   Claude API 호출    →   부드럽게 전환
씬 타입 분류          (대화면 CloseUp           (Blend / Cut /
(대화 / 액션 /         액션이면 Wide 등)          Fade)
 탐험 / 사망 등)
      │
      ▼
  로그 저장
(State, Action, Reward)
      │
      ▼
 강화학습 훈련
 (PPO 자체 모델)
      │
      ▼
 로컬 서버로 서빙
 (API 없이 동작)
```

---

## 주요 기능

- **13가지 샷 타입 자동 선택**
  ExtremeWide · Wide · Medium · MediumCloseUp · CloseUp · ExtremeCloseUp
  OverShoulder · TwoShot · POV · BirdsEye · LowAngle · Dutch

- **씬 타입 인식**
  대화(Dialogue) · 액션(Combat) · 탐험(Exploration) · 컷씬(Cutscene) · 사망(Death) · 승리(Victory)

- **3단계 AI 모드**
  Claude API → 학습된 로컬 PPO 모델 → 규칙 기반 폴백 (API 없이도 동작)

- **강화학습 데이터 자동 수집**
  모든 결정을 `(씬 상태, 샷 결정, 품질 점수)` 형태의 JSONL로 자동 저장

- **블루프린트 친화적 설계**
  C++ 없이 액터 배치 + 인터페이스 구현만으로 사용 가능

---

## 시스템 아키텍처

```
┌──────────────────────────────────────────────────────────────┐
│                       Unreal Engine 5                         │
│                                                              │
│   SceneAnalyzer ──▶ AIDirector ──▶ CineAICameraActor         │
│   (캐릭터 추적       (Claude API /    (목표 위치 계산 +       │
│    씬 타입 분류)      로컬 서버 /      카메라 이동/블렌딩)     │
│                       규칙 폴백)                              │
│                           │                                  │
│                    ExperienceLogger                          │
│                    (JSONL 자동 저장)                          │
└───────────────────────────┼──────────────────────────────────┘
                            │  Saved/CineAILogs/*.jsonl
                            ▼
┌──────────────────────────────────────────────────────────────┐
│                    Python ML Pipeline                         │
│                                                              │
│  collector.py ──▶ preprocessor.py ──▶ trainer.py (PPO)       │
│  (로그 파싱)       (정규화 / 분리)     (강화학습 훈련)         │
│                                            │                 │
│                                    inference/server.py       │
│                                    (로컬 HTTP 서버 :8765)     │
└──────────────────────────────────────────────────────────────┘
```

---

## 파일 구조

```
Cine-AI-Director/
│
├── CineAIDirector.uproject             UE5 프로젝트 파일 (더블클릭으로 열기)
├── Config/                             UE5 기본 설정
├── Content/                            레벨, 캐릭터 BP 등 프로젝트 애셋
│
├── Plugins/CineAIDirector/             AI 카메라 디렉터 플러그인
│   ├── CineAIDirector.uplugin
│   ├── Content/                        플러그인 공용 애셋 (BP_AIDirector 등)
│   └── Source/CineAIDirector/          UE5 C++ (코드 완성)
│       ├── Public/
│       │   ├── Data/
│       │   │   ├── ShotTypes.h         샷/씬 타입 enum 정의
│       │   │   ├── SceneContext.h      씬 상태 데이터 구조체  ←─┐ UE5↔ML
│       │   │   └── DirectorDecision.h  AI 결정 데이터 구조체     │ 경계
│       │   ├── Camera/                                           │
│       │   │   ├── CineAICameraActor.h 레벨에 배치할 카메라 액터  │
│       │   │   ├── ShotBlender.h       카메라 전환 블렌딩        │
│       │   │   └── ICineCharacterInterface.h  캐릭터 BP 인터페이스
│       │   ├── Director/
│       │   │   ├── AIDirector.h        레벨에 배치할 AI 디렉터 액터
│       │   │   └── SceneAnalyzer.h     씬 분석 컴포넌트
│       │   └── Logging/
│       │       ├── ExperienceLogger.h  학습 데이터 JSONL 로거  ←─┘
│       │       └── RewardEvaluator.h   품질 점수(리워드) 계산
│       └── Private/                    구현 (.cpp)
│
├── ML/src/
│   ├── data_pipeline/
│   │   ├── collector.py                UE5 로그 → DataFrame
│   │   ├── preprocessor.py             정규화 / 학습-검증 분리
│   │   └── reward_model.py             Python 리워드 함수
│   ├── training/
│   │   ├── environment.py              Gymnasium RL 환경
│   │   └── trainer.py                  PPO 학습 진입점
│   ├── inference/
│   │   ├── predictor.py                모델 로드 & 샷 예측
│   │   └── server.py                   FastAPI 로컬 추론 서버
│   ├── evaluation/
│   │   └── visualizer.py               학습 결과 시각화
│   └── run_pipeline.py                 전체 파이프라인 실행
│
├── ML/data/                            학습 데이터 (로그 원본 / 전처리 결과)
├── ML/models/                          학습된 모델 (checkpoints / exports)
└── Docs/프로젝트_가이드.md             전체 가이드
```

---

## 빠른 시작

### UE5 담당

1. 레포 클론 후 **`CineAIDirector.uproject`** 더블클릭 → UE5 열림
2. C++ 컴파일 완료 후, 레벨에 **CineAICameraActor** 배치
3. 레벨에 **AIDirector** 배치 → Details > `Target Camera` 에 위 카메라 연결
4. (선택) Details > `API Key` 에 Claude 키 입력 — 없으면 규칙 기반 자동 작동
5. Play
6. 만든 블루프린트/애셋은 `Plugins/CineAIDirector/Content/` 에 저장하면 깃에 포함됨

### ML 담당

```bash
cd ML
pip install -r requirements.txt

# UE5 데이터 없이 즉시 synthetic 학습
python src/run_pipeline.py --mode synthetic --timesteps 500000

# 로컬 추론 서버 실행 (학습 후)
python -m src.inference.server
# → UE5 AIDirector > bUseLocalInferenceServer = true 설정
```

---

## UE5 ↔ ML 동기화 필요 파일

한쪽을 수정하면 **반드시 상대방에게 알리고** 반대쪽도 함께 수정하세요.

| UE5 | Python | 동기화 항목 |
|-----|--------|------------|
| `Data/ShotTypes.h` | `reward_model.py` | ShotType / SceneType enum 숫자 |
| `Data/SceneContext.h` | `preprocessor.py` STATE_COLUMNS | 씬 상태 필드 이름·순서 |
| `Logging/ExperienceLogger.cpp` | `collector.py` | JSONL 필드 구조 |
| `Logging/RewardEvaluator.cpp` | `reward_model.py` | 리워드 가중치 ⚠️ |

> **⚠️ 동기화 필요:** `reward_model.py` 가중치가 `shot_score * 0.7 + duration_score * 0.3` 으로 업데이트됨.
> UE5 열면 `RewardEvaluator.cpp` 의 동일 수식도 반드시 맞춰야 함.

---

## 기술 스택

| | 기술 |
|-|------|
| 엔진 | Unreal Engine 5.3+ |
| 언어 | C++17 · Python 3.11 |
| AI API | Anthropic Claude (claude-opus-4-6) |
| 강화학습 | Stable-Baselines3 PPO · Gymnasium |
| 추론 서버 | FastAPI · uvicorn |
| 딥러닝 | PyTorch |
| 데이터 | JSONL |

---

## 팀

| 역할 | 담당 |
|------|------|
| ML · AI API · 강화학습 파이프라인 | ML 담당 |
| UE5 · 카메라 · 씬 구성 · 애셋 | UE5 담당 |

---

## 📖 [프로젝트 가이드 보기](Docs/프로젝트_가이드.md)

각 파일의 역할, UE5 단계별 세팅 방법, 차후 작업 목록이 정리되어 있습니다.
