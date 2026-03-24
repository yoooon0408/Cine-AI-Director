"""
OpenAI Gymnasium 환경 - UE5 카메라 디렉터 시뮬레이션

[ML 담당] RL 에이전트가 학습할 환경입니다.

두 가지 모드:
  synthetic : 랜덤 씬 컨텍스트를 생성하고, 에이전트 행동에 기반해 리워드 계산.
              UE5 데이터 없이도 즉시 학습 가능. 개발/디버깅에 사용.
  dataset   : UE5에서 수집된 상태를 순서대로 재생하고, 에이전트 행동에 기반해
              리워드를 재계산. (저장된 reward를 그냥 쓰지 않음에 주의)

State  : 씬 컨텍스트 벡터 7차원 (정규화됨)
Action : 샷 타입 선택 (Discrete(13))
Reward : _compute_reward(scene_type, shot_type, shot_duration)
"""

import numpy as np
import gymnasium as gym
from gymnasium import spaces
from pathlib import Path

from data_pipeline.reward_model import (
    SCENE_PREFERRED_SHOTS,
    _score_shot_duration,
    SceneType,
)


NUM_SHOT_TYPES  = 13   # ShotTypes.h의 ECineShotType 항목 수와 동일하게 유지
OBS_DIM         = 7    # SceneContext 벡터 차원
EPISODE_LENGTH  = 200  # synthetic mode 에피소드 길이


class CineDirectorEnv(gym.Env):
    """카메라 디렉터 강화학습 환경"""

    metadata = {"render_modes": []}

    def __init__(
        self,
        mode: str = "synthetic",
        dataset_path: Path = None,
        episode_length: int = EPISODE_LENGTH,
    ):
        """
        Args:
            mode          : "synthetic" 또는 "dataset"
            dataset_path  : dataset 모드 시 processed 데이터 디렉터리
            episode_length: synthetic 모드의 에피소드 길이
        """
        super().__init__()
        assert mode in ("synthetic", "dataset"), f"Unknown mode: {mode}"

        self._mode           = mode
        self._episode_length = episode_length

        self.action_space = spaces.Discrete(NUM_SHOT_TYPES)
        self.observation_space = spaces.Box(
            low=-np.inf, high=np.inf, shape=(OBS_DIM,), dtype=np.float32
        )

        # 현재 스텝에서 reward 계산에 필요한 raw 값
        self._current_scene_type    = SceneType.UNKNOWN
        self._current_shot_duration = 0.0
        self._step_count            = 0

        # dataset 모드 전용
        self._states_norm = None   # 정규화된 state (obs용)
        self._states_raw  = None   # 원본 state (reward 계산용)
        self._current_idx = 0

        if mode == "dataset":
            assert dataset_path is not None, "dataset 모드는 dataset_path가 필요합니다"
            self._load_dataset(dataset_path)

    # ------------------------------------------------------------------
    # Gym interface
    # ------------------------------------------------------------------

    def reset(self, seed=None, options=None):
        super().reset(seed=seed)
        self._step_count  = 0
        self._current_idx = 0
        obs = self._next_obs()
        return obs, {}

    def step(self, action: int):
        # 핵심: 저장된 reward를 쓰지 않고, (state, action) 쌍으로 직접 계산
        reward = self._compute_reward(
            self._current_scene_type,
            action,
            self._current_shot_duration,
        )
        self._step_count  += 1
        self._current_idx += 1

        if self._mode == "dataset":
            terminated = self._current_idx >= len(self._states_norm)
        else:
            terminated = self._step_count >= self._episode_length

        obs = self._next_obs()
        return obs, reward, terminated, False, {}

    # ------------------------------------------------------------------
    # Internal helpers
    # ------------------------------------------------------------------

    def _load_dataset(self, path: Path):
        self._states_norm = np.load(path / "train_states.npy").astype(np.float32)
        self._states_raw  = np.load(path / "train_states_raw.npy").astype(np.float32)
        print(f"[env] 데이터셋 로드: {len(self._states_norm)}개 샘플")

    def _next_obs(self) -> np.ndarray:
        """다음 observation을 반환하고 reward 계산용 내부 상태를 갱신한다."""
        if self._mode == "dataset":
            if self._current_idx < len(self._states_norm):
                raw = self._states_raw[self._current_idx]
                self._current_scene_type    = int(raw[0])    # state_scene_type
                self._current_shot_duration = float(raw[3])  # state_shot_duration
                return self._states_norm[self._current_idx]
            return np.zeros(OBS_DIM, dtype=np.float32)
        else:
            return self._sample_synthetic_obs()

    def _sample_synthetic_obs(self) -> np.ndarray:
        """synthetic 모드: 유효한 랜덤 씬 컨텍스트를 생성한다."""
        scene_type   = int(self.np_random.integers(1, 7))              # 1-6
        char_count   = int(self.np_random.integers(1, 6))              # 1-5
        current_shot = int(self.np_random.integers(0, NUM_SHOT_TYPES)) # 0-12
        duration     = float(self.np_random.uniform(0.0, 20.0))
        cam_x        = float(self.np_random.uniform(-1000.0, 1000.0))
        cam_y        = float(self.np_random.uniform(-1000.0, 1000.0))
        cam_z        = float(self.np_random.uniform(0.0, 500.0))

        self._current_scene_type    = scene_type
        self._current_shot_duration = duration

        # 정규화 (dataset 모드의 전처리와 동일한 스케일로 맞춤)
        obs = np.array(
            [
                scene_type   / 6.0,
                char_count   / 5.0,
                current_shot / 12.0,
                duration     / 20.0,
                cam_x        / 1000.0,
                cam_y        / 1000.0,
                cam_z        / 500.0,
            ],
            dtype=np.float32,
        )
        return obs

    def _compute_reward(
        self, scene_type: int, shot_type: int, shot_duration: float
    ) -> float:
        """
        (state, action) 쌍으로 리워드를 계산한다.
        UE5 RewardEvaluator.cpp 및 reward_model.py와 동일한 가중치 사용.
        """
        preferred      = SCENE_PREFERRED_SHOTS.get(scene_type, [])
        shot_score     = 1.0 if shot_type in preferred else 0.0
        duration_score = _score_shot_duration(shot_duration)
        reward = shot_score * 0.7 + duration_score * 0.3
        return float(np.clip(reward, -1.0, 1.0))
