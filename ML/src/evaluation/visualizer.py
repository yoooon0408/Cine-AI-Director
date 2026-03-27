"""
학습 결과 시각화

[ML 담당] 포트폴리오용 시각화입니다.
          - 샷 타입 분포
          - 씬 상황별 에이전트 선택 경향
          - 리워드 곡선
"""

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.font_manager as fm
from pathlib import Path

from data_pipeline.reward_model import ShotType, SceneType


SHOT_NAMES = {
    0: "None", 1: "ExtremeWide", 2: "Wide", 3: "Medium",
    4: "MediumCU", 5: "CloseUp", 6: "ExtremeCU", 7: "OverShoulder",
    8: "TwoShot", 9: "POV", 10: "BirdsEye", 11: "LowAngle", 12: "Dutch",
}

SCENE_NAMES = {
    0: "Unknown", 1: "Dialogue", 2: "Combat",
    3: "Exploration", 4: "Cutscene", 5: "Death", 6: "Victory",
}


def plot_shot_distribution(df: pd.DataFrame, save_path: Path = None):
    """샷 타입 선택 분포 시각화"""
    counts = df["action_shot_type"].value_counts().sort_index()

    fig, ax = plt.subplots(figsize=(12, 5))
    labels = [SHOT_NAMES.get(int(i), str(i)) for i in counts.index]
    ax.bar(labels, counts.values, color="steelblue")
    ax.set_title("Shot Type Distribution")
    ax.set_xlabel("Shot Type")
    ax.set_ylabel("Count")
    plt.xticks(rotation=30, ha="right")
    plt.tight_layout()

    if save_path:
        plt.savefig(save_path, dpi=150)
        plt.close()
    else:
        plt.show()


def plot_reward_by_scene(df: pd.DataFrame, save_path: Path = None):
    """씬 타입별 평균 리워드 시각화"""
    grouped = df.groupby("state_scene_type")["reward"].mean()

    fig, ax = plt.subplots(figsize=(8, 5))
    labels = [SCENE_NAMES.get(int(i), str(i)) for i in grouped.index]
    ax.bar(labels, grouped.values, color="darkorange")
    ax.set_title("Mean Reward by Scene Type")
    ax.set_xlabel("Scene Type")
    ax.set_ylabel("Mean Reward")
    ax.axhline(0, color="black", linewidth=0.8, linestyle="--")
    plt.tight_layout()

    if save_path:
        plt.savefig(save_path, dpi=150)
        plt.close()
    else:
        plt.show()


def plot_reward_curve(rewards: list[float], save_path: Path = None):
    """에피소드별 리워드 커브"""
    fig, ax = plt.subplots(figsize=(10, 4))
    ax.plot(rewards, alpha=0.6, color="green", label="Reward")

    # 이동 평균
    window = min(50, len(rewards) // 10 + 1)
    if len(rewards) >= window:
        moving_avg = pd.Series(rewards).rolling(window).mean()
        ax.plot(moving_avg, color="darkgreen", linewidth=2, label=f"MA-{window}")

    ax.set_title("Training Reward Curve")
    ax.set_xlabel("Episode")
    ax.set_ylabel("Reward")
    ax.legend()
    plt.tight_layout()

    if save_path:
        plt.savefig(save_path, dpi=150)
        plt.close()
    else:
        plt.show()
