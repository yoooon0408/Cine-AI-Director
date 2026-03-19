"""
수집된 원본 데이터를 RL 학습에 맞는 형태로 전처리합니다.

[ML 담당] State/Action을 정규화하고, 학습용/검증용으로 분리합니다.
"""

import numpy as np
import pandas as pd
from pathlib import Path


# State 컬럼 목록 (UE5 SceneContext의 필드와 대응)
STATE_COLUMNS = [
    "state_scene_type",
    "state_character_count",
    "state_current_shot",
    "state_shot_duration",
    "state_cam_x",
    "state_cam_y",
    "state_cam_z",
]

# Action 컬럼 목록 (UE5 DirectorDecision의 필드와 대응)
ACTION_COLUMNS = [
    "action_shot_type",
    "action_transition_type",
    "action_transition_duration",
]


def preprocess(df: pd.DataFrame, train_ratio: float = 0.8) -> tuple[dict, dict]:
    """
    원본 DataFrame을 학습/검증 세트로 전처리합니다.

    Returns:
        (train_data, val_data) - 각각 {"states", "states_raw", "actions", "rewards"} 딕셔너리
        states_raw : 정규화 전 원본 값. 환경에서 reward 계산 시 scene_type 등을 복원하는 데 사용.
    """
    df = df.dropna(subset=STATE_COLUMNS + ACTION_COLUMNS + ["reward"])

    states_raw = df[STATE_COLUMNS].values.astype(np.float32)  # 정규화 전 저장
    states     = _normalize(states_raw)
    actions    = df[ACTION_COLUMNS].values.astype(np.float32)
    rewards    = df["reward"].values.astype(np.float32)

    # 분리
    n = len(df)
    split = int(n * train_ratio)

    train = {
        "states":     states[:split],
        "states_raw": states_raw[:split],
        "actions":    actions[:split],
        "rewards":    rewards[:split],
    }
    val = {
        "states":     states[split:],
        "states_raw": states_raw[split:],
        "actions":    actions[split:],
        "rewards":    rewards[split:],
    }

    print(f"[preprocessor] 학습: {split}개 / 검증: {n - split}개")
    return train, val


def _normalize(states: np.ndarray) -> np.ndarray:
    mean = states.mean(axis=0)
    std  = states.std(axis=0) + 1e-8
    return (states - mean) / std


def save_processed(train: dict, val: dict, output_dir: Path) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)
    for split_name, data in [("train", train), ("val", val)]:
        for key, arr in data.items():
            np.save(output_dir / f"{split_name}_{key}.npy", arr)
    print(f"[preprocessor] 저장 완료: {output_dir}")
