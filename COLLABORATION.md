# 협업 가이드

## 담당 영역

| 영역 | 담당 | 경로 |
|------|------|------|
| UE5 카메라/씬 분석 | 언리얼 담당 | `Source/CineAIDirector/` |
| 에디터 UI | 언리얼 담당 | `Source/CineAIDirectorEditor/` |
| AI API 연동 | ML 담당 | `Source/CineAIDirector/Private/Director/AIDirector.cpp` |
| 데이터 로깅 | ML 담당 | `Source/CineAIDirector/*/Logging/` |
| RL 훈련 파이프라인 | ML 담당 | `ML/` |

---

## 핵심 경계선 (두 사람이 함께 봐야 하는 파일들)

### `Source/CineAIDirector/Public/Data/SceneContext.h`
- UE5 → ML로 전달되는 **씬 상태 구조체**
- 필드를 추가/변경하면 **반드시 ML 담당에게 알릴 것**
- ML 쪽 `preprocessor.py`의 `STATE_COLUMNS`도 같이 수정해야 함

### `Source/CineAIDirector/Public/Data/ShotTypes.h`
- 샷 타입 enum이 정의된 파일
- 항목 추가 시 ML 쪽 `reward_model.py`의 `ShotType`, `SCENE_PREFERRED_SHOTS`도 동기화 필요

### `Source/CineAIDirector/Public/Logging/ExperienceLogger.h`
- UE5가 ML 파이프라인에 데이터를 넘기는 **연결 지점**
- 로그 포맷 변경 시 `ML/src/data_pipeline/collector.py`도 수정 필요

---

## 언리얼 담당이 주로 작업하는 파일들

```
Source/CineAIDirector/Public/Camera/       ← 카메라 액터/컴포넌트
Source/CineAIDirector/Private/Camera/
Source/CineAIDirector/Private/Director/SceneAnalyzer.cpp  ← 씬 분류 로직
Source/CineAIDirectorEditor/               ← 에디터 UI
```

**UE5에서 시작하는 법:**
1. `.uplugin` 파일을 UE5 프로젝트의 `Plugins/` 폴더에 복사
2. 레벨에 `ACineAICameraActor` 배치
3. 레벨에 `AAIDirector` 배치하고 Details 패널에서 카메라 연결
4. Play → 자동으로 작동

---

## ML 담당이 주로 작업하는 파일들

```
Source/CineAIDirector/Private/Director/AIDirector.cpp  ← API 호출/파싱
Source/CineAIDirector/*/Logging/                       ← 로그 포맷
ML/src/                                                ← 전체 학습 파이프라인
```

**ML 파이프라인 실행 순서:**
```bash
cd ML
pip install -r requirements.txt

# 1. UE5에서 게임 플레이 → Saved/CineAILogs/*.jsonl 생성됨
# 2. 데이터 수집 확인
python -c "from src.data_pipeline.collector import load_experiences; print(load_experiences())"

# 3. 전처리
# (preprocessor.py 실행)

# 4. 훈련
python src/training/trainer.py
```

---

## 브랜치 전략

- `main` : 안정적인 통합 버전
- `feature/ue5-*` : 언리얼 담당 작업
- `feature/ml-*` : ML 담당 작업
- PR 머지 전에 서로 리뷰 요청하기

---

## 자주 하는 실수

| 실수 | 해결 |
|------|------|
| ShotType enum에 값 추가했는데 reward_model.py가 모름 | 두 파일 동시 수정 |
| SceneContext에 필드 추가했는데 collector.py가 빠뜨림 | STATE_COLUMNS 업데이트 |
| API 키를 코드에 하드코딩 | `.env` 파일 사용, `.gitignore`에 추가 |
