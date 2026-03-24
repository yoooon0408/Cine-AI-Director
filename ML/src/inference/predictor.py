"""
학습된 PPO 모델을 로드하고 씬 컨텍스트로부터 샷 타입을 예측합니다.

[ML 담당] trainer.py로 학습이 완료되면 이 모듈을 통해 모델을 서빙합니다.

사용 예:
    predictor = CineDirectorPredictor()
    predictor.load()
    result = predictor.predict({
        "scene_type": 1,
        "character_count": 2,
        "current_shot": 3,
        "shot_duration": 5.0,
        "cam_x": 0.0, "cam_y": 0.0, "cam_z": 150.0,
    })
    print(result)
    # {"shot_type": 5, "confidence": 0.82, "transition_type": 1, "transition_duration": 0.8}
"""

import numpy as np
from pathlib import Path
from typing import Optional


# STATE_COLUMNS 순서는 preprocessor.py 와 동일하게 유지해야 합니다.
STATE_COLUMNS = [
    "scene_type",
    "character_count",
    "current_shot",
    "shot_duration",
    "cam_x",
    "cam_y",
    "cam_z",
]

# 정규화 스케일 (environment.py _sample_synthetic_obs 와 동일한 스케일)
# dataset 모드로 학습했다면 실제 통계치로 교체하세요.
NORMALIZATION_SCALE = np.array(
    [6.0, 5.0, 12.0, 20.0, 1000.0, 1000.0, 500.0],
    dtype=np.float32,
)

# 씬 타입별 권장 전환 방식 (transition_type: 0=Cut, 1=Blend, 2=Fade)
SCENE_TRANSITION_TYPE = {
    1: 1,  # Dialogue → Blend
    2: 0,  # Combat   → Cut (빠른 전환)
    3: 1,  # Exploration → Blend
    4: 2,  # Cutscene → Fade
    5: 2,  # Death    → Fade
    6: 1,  # Victory  → Blend
}

# 씬 타입별 권장 전환 시간 (초)
SCENE_TRANSITION_DURATION = {
    1: 0.8,
    2: 0.3,
    3: 1.0,
    4: 1.5,
    5: 2.0,
    6: 1.2,
}

DEFAULT_MODEL_PATH = Path(__file__).parents[2] / "models" / "exports" / "cine_director_final.zip"


class CineDirectorPredictor:
    """학습된 PPO 모델로 카메라 샷 타입을 예측하는 클래스"""

    def __init__(self, model_path: Optional[Path] = None):
        self._model_path = model_path or DEFAULT_MODEL_PATH
        self._model = None

    def load(self) -> bool:
        """
        모델을 로드합니다.

        Returns:
            True: 로드 성공
            False: 모델 파일 없음 또는 로드 실패 (서버는 폴백 모드로 계속 작동)
        """
        if not self._model_path.exists():
            print(
                f"[predictor] 모델 파일을 찾을 수 없습니다: {self._model_path}\n"
                f"  → trainer.py를 먼저 실행하여 모델을 학습시켜 주세요.\n"
                f"  → 서버는 규칙 기반 폴백 모드로 작동합니다."
            )
            return False

        try:
            from stable_baselines3 import PPO

            self._model = PPO.load(str(self._model_path))
            print(f"[predictor] 모델 로드 완료: {self._model_path}")
            return True
        except Exception as e:
            print(f"[predictor] 모델 로드 실패: {e}")
            self._model = None
            return False

    @property
    def is_loaded(self) -> bool:
        return self._model is not None

    def predict(self, state_dict: dict) -> dict:
        """
        씬 컨텍스트 딕셔너리를 받아 샷 결정을 반환합니다.

        Args:
            state_dict: UE5가 보내는 씬 상태 (scene_type, character_count, ...)

        Returns:
            {shot_type, confidence, transition_type, transition_duration, reasoning}
        """
        scene_type = int(state_dict.get("scene_type", 0))

        if self._model is None:
            return self._fallback_predict(scene_type)

        obs = self._build_observation(state_dict)
        action, _ = self._model.predict(obs, deterministic=True)
        shot_type = int(action)

        # confidence: action probability 추출 (SB3 내부 정책 활용)
        confidence = self._estimate_confidence(obs)

        return {
            "shot_type":          shot_type,
            "confidence":         confidence,
            "transition_type":    SCENE_TRANSITION_TYPE.get(scene_type, 1),
            "transition_duration": SCENE_TRANSITION_DURATION.get(scene_type, 0.8),
            "reasoning":          f"PPO inference - scene={scene_type}, shot={shot_type}",
        }

    def _build_observation(self, state_dict: dict) -> np.ndarray:
        """state_dict → 정규화된 numpy 벡터 (environment.py와 동일한 스케일)"""
        raw = np.array(
            [state_dict.get(col, 0.0) for col in STATE_COLUMNS],
            dtype=np.float32,
        )
        obs = raw / NORMALIZATION_SCALE
        return obs

    def _estimate_confidence(self, obs: np.ndarray) -> float:
        """
        PPO 정책의 action 확률 분포에서 최댓값을 confidence로 사용합니다.
        """
        try:
            import torch
            obs_tensor = torch.tensor(obs, dtype=torch.float32).unsqueeze(0)
            with torch.no_grad():
                dist = self._model.policy.get_distribution(obs_tensor)
                probs = dist.distribution.probs.squeeze(0).numpy()
            return float(probs.max())
        except Exception:
            return 0.6  # 추출 실패 시 기본값

    def _fallback_predict(self, scene_type: int) -> dict:
        """모델 없을 때 규칙 기반 폴백"""
        # 씬 타입별 기본 샷 선택
        fallback_shots = {
            1: 4,   # Dialogue   → MediumCloseUp
            2: 2,   # Combat     → Wide
            3: 3,   # Exploration → Medium
            4: 3,   # Cutscene   → Medium
            5: 5,   # Death      → CloseUp
            6: 2,   # Victory    → Wide
        }
        shot_type = fallback_shots.get(scene_type, 3)

        return {
            "shot_type":           shot_type,
            "confidence":          0.4,
            "transition_type":     SCENE_TRANSITION_TYPE.get(scene_type, 1),
            "transition_duration": SCENE_TRANSITION_DURATION.get(scene_type, 0.8),
            "reasoning":           f"Fallback (no model) - scene={scene_type}, shot={shot_type}",
        }
