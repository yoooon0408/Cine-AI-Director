"""
OpenAI Gymnasium 환경 - UE5 카메라 디렉터 시뮬레이션

[ML 담당] RL 에이전트가 학습할 환경입니다.
          오프라인 데이터셋 기반의 배치 RL 환경으로 구현합니다.
          (UE5와 실시간 연동이 아닌, 수집된 경험 데이터로 학습)
"""

import numpy as np
import gymnasium as gym
from gymnasium import spaces
from pathlib import Path

from data_pipeline.reward_model import ShotType, SceneType


# Action Space 크기 (ECineShotType의 항목 수와 동일하게 유지)
NUM_SHOT_TYPES = 13  # ShotTypes.h의 ECineShotType 값 수


class CineDirectorEnv(gym.Env):
    """
    카메라 디렉터 강화학습 환경

    State:  씬 컨텍스트 벡터 (SceneContext 기반)
    Action: 샷 타입 선택 (Discrete)
    Reward: RewardEvaluator 기준
    """

    metadata = {"render_modes": []}

    def __init__(self, dataset_path: Path = None):
        super().__init__()

        # Action Space: 샷 타입 선택 (Discrete)
        self.action_space = spaces.Discrete(NUM_SHOT_TYPES)

        # Observation Space: 씬 컨텍스트 벡터
        # [scene_type, character_count, current_shot, shot_duration, cam_x, cam_y, cam_z]
        self.observation_space = spaces.Box(
            low=-np.inf,
            high=np.inf,
            shape=(7,),
            dtype=np.float32,
        )

        self._dataset = None
        self._current_idx = 0

        if dataset_path:
            self._load_dataset(dataset_path)

    def _load_dataset(self, path: Path):
        self._states  = np.load(path / "train_states.npy")
        self._rewards = np.load(path / "train_rewards.npy")
        self._dataset = True
        print(f"[env] 데이터셋 로드: {len(self._states)}개 샘플")

    def reset(self, seed=None, options=None):
        super().reset(seed=seed)
        self._current_idx = 0
        obs = self._get_obs()
        return obs, {}

    def step(self, action: int):
        reward = float(self._rewards[self._current_idx]) if self._dataset else 0.0
        self._current_idx += 1

        terminated = self._current_idx >= len(self._states) if self._dataset else False
        obs = self._get_obs()

        return obs, reward, terminated, False, {}

    def _get_obs(self) -> np.ndarray:
        if self._dataset and self._current_idx < len(self._states):
            return self._states[self._current_idx].astype(np.float32)
        return np.zeros(7, dtype=np.float32)
