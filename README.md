# Cine AI Director

> AI-powered autonomous cinematic camera system for Unreal Engine 5

게임 내 상황을 실시간으로 분석해 영화 감독처럼 최적의 카메라 앵글을 자동으로 선택하는 UE5 플러그인입니다.
외부 AI API(Claude)로 고품질 판단을 수행하고, 수집된 데이터로 자체 강화학습 모델을 훈련합니다.

---

## 주요 기능

- **실시간 씬 분석** — 캐릭터 위치/속도/상태를 매 프레임 추적
- **AI 기반 샷 선택** — Claude API가 씬 컨텍스트를 보고 최적 샷 타입 결정
- **13가지 샷 타입** — Wide, CloseUp, OverShoulder, POV, BirdsEye 등
- **부드러운 트랜지션** — Cut / Blend / Fade 방식의 카메라 전환
- **RL 데이터 파이프라인** — 모든 결정을 (State, Action, Reward)로 자동 로깅
- **자체 모델 훈련** — 수집 데이터로 PPO 기반 강화학습 에이전트 훈련

---

## 시스템 아키텍처

```
┌─────────────────────────────────────────────────────────┐
│                    Unreal Engine 5                       │
│                                                         │
│  SceneAnalyzer ──→ AIDirector ──→ CineAICameraActor     │
│  (씬 상태 수집)    (API 호출)     (카메라 이동/블렌딩)   │
│                        │                                │
│                   ExperienceLogger                      │
│                  (State, Action, Reward 저장)            │
└────────────────────────┼────────────────────────────────┘
                         │ JSONL 로그
                         ▼
┌─────────────────────────────────────────────────────────┐
│                  Python ML Pipeline                      │
│                                                         │
│  collector.py → preprocessor.py → trainer.py (PPO)      │
│  (로그 파싱)     (전처리/정규화)   (강화학습 훈련)        │
└─────────────────────────────────────────────────────────┘
```

---

## 팀 구성

| 담당 | 역할 |
|------|------|
| ML 담당 | Claude API 연동, 강화학습 파이프라인, 리워드 설계 |
| UE5 담당 | 카메라 시스템, 씬 분석, 레벨/에셋, 에디터 UI |

---

## 프로젝트 구조

```
CineAIDirector/
├── Source/
│   ├── CineAIDirector/            # 런타임 플러그인 (C++)
│   │   ├── Public/
│   │   │   ├── Camera/            # CineAICameraActor, ShotBlender
│   │   │   ├── Director/          # AIDirector, SceneAnalyzer
│   │   │   ├── Data/              # ShotTypes, SceneContext, DirectorDecision
│   │   │   └── Logging/           # ExperienceLogger, RewardEvaluator
│   │   └── Private/               # 구현 (.cpp)
│   └── CineAIDirectorEditor/      # 에디터 전용 모듈
│
├── ML/                            # Python RL 파이프라인
│   ├── src/
│   │   ├── data_pipeline/         # collector, preprocessor, reward_model
│   │   ├── training/              # environment (Gym), trainer (PPO)
│   │   └── evaluation/            # visualizer
│   ├── data/                      # raw 로그 / processed 데이터
│   └── models/                    # checkpoints / exports
│
└── Docs/                          # 협업 가이드 문서
```

---

## 시작하기

### UE5 담당

1. UE5 프로젝트 생성 (Third Person 템플릿 권장)
2. `Plugins/` 폴더에 이 플러그인 복사
3. 레벨에 `CineAI Camera` + `Cine AI Director` 액터 배치
4. Director의 Target Camera 연결 후 Play

자세한 내용 → [UE5 시작가이드](Docs/01_UE5_시작가이드.md) | [테스트환경 구축가이드](Docs/03_테스트환경_구축가이드.md)

### ML 담당

```bash
cd ML
python -m venv venv && source venv/Scripts/activate
pip install -r requirements.txt

# .env 파일에 API 키 설정
echo "ANTHROPIC_API_KEY=sk-ant-..." > .env

# UE5 플레이 후 수집된 데이터로 훈련
python src/training/trainer.py
```

자세한 내용 → [ML 시작가이드](Docs/02_ML_시작가이드.md)

---

## UE5 ↔ ML 연결 지점

두 담당자가 함께 관리해야 하는 파일들입니다.
한쪽을 수정하면 반드시 상대방에게 알리고 반대쪽도 동기화하세요.

| UE5 파일 | Python 파일 | 동기화 항목 |
|----------|------------|------------|
| `Data/ShotTypes.h` | `data_pipeline/reward_model.py` | Shot/Scene enum 값 |
| `Data/SceneContext.h` | `data_pipeline/preprocessor.py` | STATE_COLUMNS 필드 |
| `Logging/ExperienceLogger.cpp` | `data_pipeline/collector.py` | JSON 로그 포맷 |
| `Logging/RewardEvaluator.cpp` | `data_pipeline/reward_model.py` | 리워드 가중치 |

---

## 기술 스택

| 분류 | 기술 |
|------|------|
| 게임 엔진 | Unreal Engine 5.3+ |
| 언어 (UE5) | C++17 |
| AI API | Anthropic Claude (claude-opus-4-6) |
| 언어 (ML) | Python 3.11 |
| 강화학습 | Stable-Baselines3 (PPO), Gymnasium |
| 딥러닝 | PyTorch |
| 데이터 포맷 | JSONL (UE5 → Python 로그) |

---

## 가이드 문서

| 문서 | 대상 |
|------|------|
| [00 전체 협업가이드북](Docs/00_전체_협업가이드북.md) | 모두 |
| [01 UE5 시작가이드](Docs/01_UE5_시작가이드.md) | UE5 담당 |
| [02 ML 시작가이드](Docs/02_ML_시작가이드.md) | ML 담당 |
| [03 테스트환경 구축가이드](Docs/03_테스트환경_구축가이드.md) | UE5 담당 |
