"""
UE5 ExperienceLogger가 출력한 JSONL 파일을 읽어서 학습 데이터로 변환합니다.

[ML 담당] 이 파일이 UE5 → Python 파이프라인의 입구입니다.
          UE5의 ExperienceLogger가 두 종류의 파일을 생성합니다:
            - session_<ID>.jsonl        : (state, action, reward=0) 원본 로그
            - reward_updates_<ID>.jsonl : 사후 reward 업데이트 패치

          두 파일을 병합해 최종 DataFrame을 반환합니다.
"""

import json
import pandas as pd
from pathlib import Path


# UE5 ExperienceLogger 출력 경로 (프로젝트 루트 기준)
DEFAULT_LOG_DIR = Path(__file__).parents[3] / "Saved" / "CineAILogs"


def load_experiences(log_dir: Path = DEFAULT_LOG_DIR) -> pd.DataFrame:
    """
    JSONL 로그 파일을 모두 읽어 DataFrame으로 반환합니다.

    reward_updates_*.jsonl 가 있으면 자동으로 reward 값을 패치합니다.

    Returns:
        컬럼: id, timestamp, reward, state_*, action_*
    """
    log_dir = Path(log_dir)
    experience_files = list(log_dir.glob("session_*.jsonl")) + \
                       list(log_dir.glob("experience_log.jsonl"))

    if not experience_files:
        raise FileNotFoundError(
            f"경험 로그 파일이 없습니다: {log_dir}\n"
            f"  UE5에서 게임을 실행하면 자동으로 생성됩니다."
        )

    # ── 경험 로그 로드 ────────────────────────────────────────────────
    records = []
    for file in experience_files:
        with open(file, "r", encoding="utf-8") as f:
            for line in f:
                line = line.strip()
                if not line:
                    continue
                try:
                    records.append(_flatten_entry(json.loads(line)))
                except json.JSONDecodeError:
                    continue

    df = pd.DataFrame(records)
    if df.empty:
        raise ValueError("로그 파일이 있지만 파싱 가능한 항목이 없습니다.")

    # ── 리워드 업데이트 패치 ──────────────────────────────────────────
    reward_updates = _load_reward_updates(log_dir)
    if reward_updates:
        df = _apply_reward_updates(df, reward_updates)
        print(f"[collector] 리워드 업데이트 {len(reward_updates)}건 적용")

    print(f"[collector] {len(df)}개 경험 로드 완료 ({len(experience_files)}개 파일)")
    return df


def _flatten_entry(entry: dict) -> dict:
    """중첩된 JSON을 flat dict로 변환합니다."""
    flat = {
        "id":        entry.get("id"),
        "timestamp": entry.get("timestamp", 0.0),
        "reward":    entry.get("reward", 0.0),
    }

    for k, v in entry.get("state", {}).items():
        flat[f"state_{k}"] = v

    for k, v in entry.get("action", {}).items():
        flat[f"action_{k}"] = v

    return flat


def _load_reward_updates(log_dir: Path) -> dict:
    """reward_updates_*.jsonl 파일을 읽어 {id: reward} 딕셔너리로 반환합니다."""
    updates = {}
    for file in log_dir.glob("reward_updates_*.jsonl"):
        with open(file, "r", encoding="utf-8") as f:
            for line in f:
                line = line.strip()
                if not line:
                    continue
                try:
                    entry = json.loads(line)
                    exp_id = entry.get("id")
                    reward = entry.get("reward")
                    if exp_id is not None and reward is not None:
                        updates[exp_id] = float(reward)
                except json.JSONDecodeError:
                    continue
    return updates


def _apply_reward_updates(df: pd.DataFrame, updates: dict) -> pd.DataFrame:
    """DataFrame의 reward 컬럼을 업데이트 딕셔너리로 패치합니다."""
    df = df.copy()
    mask = df["id"].isin(updates)
    df.loc[mask, "reward"] = df.loc[mask, "id"].map(updates)
    return df


if __name__ == "__main__":
    df = load_experiences()
    print(df.head())
    print(df.dtypes)
