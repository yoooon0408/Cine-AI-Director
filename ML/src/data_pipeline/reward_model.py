"""
리워드 함수 정의

[ML 담당] UE5의 RewardEvaluator.cpp와 동일한 로직을 Python에서도 정의합니다.
          오프라인 학습 시 리워드를 재계산할 때 사용합니다.
          두 파일의 리워드 로직이 일치하도록 유지해주세요.

ShotType enum (UE5 ShotTypes.h와 동일하게 유지):
    0: None
    1: ExtremeWide
    2: Wide
    3: Medium
    4: MediumCloseUp
    5: CloseUp
    6: ExtremeCloseUp
    7: OverShoulder
    8: TwoShot
    9: POV
    10: BirdsEye
    11: LowAngle
    12: Dutch

SceneType enum (UE5 ShotTypes.h와 동일하게 유지):
    0: Unknown
    1: Dialogue
    2: Combat
    3: Exploration
    4: Cutscene
    5: Death
    6: Victory
"""

import numpy as np
import pandas as pd


# ShotType 상수 (UE5 ECineShotType과 동기화)
class ShotType:
    NONE = 0
    EXTREME_WIDE = 1
    WIDE = 2
    MEDIUM = 3
    MEDIUM_CLOSEUP = 4
    CLOSEUP = 5
    EXTREME_CLOSEUP = 6
    OVER_SHOULDER = 7
    TWO_SHOT = 8
    POV = 9
    BIRDS_EYE = 10
    LOW_ANGLE = 11
    DUTCH = 12


# SceneType 상수 (UE5 ECineSceneType과 동기화)
class SceneType:
    UNKNOWN = 0
    DIALOGUE = 1
    COMBAT = 2
    EXPLORATION = 3
    CUTSCENE = 4
    DEATH = 5
    VICTORY = 6


# 씬 상황별 선호 샷 타입 (UE5 RewardEvaluator.cpp와 동기화)
SCENE_PREFERRED_SHOTS: dict[int, list[int]] = {
    SceneType.DIALOGUE:    [ShotType.CLOSEUP, ShotType.MEDIUM_CLOSEUP, ShotType.OVER_SHOULDER],
    SceneType.COMBAT:      [ShotType.WIDE, ShotType.MEDIUM],
    SceneType.EXPLORATION: [ShotType.WIDE, ShotType.EXTREME_WIDE, ShotType.BIRDS_EYE],
    SceneType.DEATH:       [ShotType.CLOSEUP, ShotType.LOW_ANGLE],
    SceneType.VICTORY:     [ShotType.WIDE, ShotType.BIRDS_EYE],
}


def compute_reward(row: pd.Series) -> float:
    """단일 경험에 대한 리워드를 계산합니다."""
    shot_score = _score_shot_appropriateness(
        int(row.get("state_scene_type", 0)),
        int(row.get("action_shot_type", 0)),
    )
    duration_score = _score_shot_duration(float(row.get("state_shot_duration", 0.0)))

    # 가중치는 UE5 RewardEvaluator.cpp와 동일하게 유지
    reward = shot_score * 0.4 + duration_score * 0.2
    return float(np.clip(reward, -1.0, 1.0))


def recompute_rewards(df: pd.DataFrame) -> pd.DataFrame:
    """DataFrame 전체에 리워드를 재계산합니다."""
    df = df.copy()
    df["reward"] = df.apply(compute_reward, axis=1)
    return df


def _score_shot_appropriateness(scene_type: int, shot_type: int) -> float:
    preferred = SCENE_PREFERRED_SHOTS.get(scene_type, [])
    if shot_type in preferred:
        return 1.0
    return 0.0


def _score_shot_duration(duration: float) -> float:
    if duration < 1.5:   return -0.5
    if duration < 3.0:   return  0.5
    if duration < 8.0:   return  1.0
    if duration < 15.0:  return  0.3
    return -0.3
