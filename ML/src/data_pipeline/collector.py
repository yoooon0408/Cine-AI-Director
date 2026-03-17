"""
UE5 ExperienceLogger가 출력한 JSONL 파일을 읽어서 학습 데이터로 변환합니다.

[ML 담당] 이 파일이 UE5 → Python 파이프라인의 입구입니다.
          UE5의 ExperienceLogger가 Saved/CineAILogs/*.jsonl 로 저장하는
          파일을 읽어서 pandas DataFrame으로 반환합니다.
"""

import json
import pandas as pd
from pathlib import Path


# UE5 ExperienceLogger의 출력 경로 (프로젝트 루트 기준)
DEFAULT_LOG_DIR = Path(__file__).parents[3] / "Saved" / "CineAILogs"


def load_experiences(log_dir: Path = DEFAULT_LOG_DIR) -> pd.DataFrame:
    """
    JSONL 로그 파일을 모두 읽어 DataFrame으로 반환합니다.

    Returns:
        컬럼: id, timestamp, reward, state_*, action_*
    """
    records = []
    log_files = list(log_dir.glob("*.jsonl"))

    if not log_files:
        raise FileNotFoundError(f"로그 파일이 없습니다: {log_dir}")

    for file in log_files:
        with open(file, "r", encoding="utf-8") as f:
            for line in f:
                line = line.strip()
                if not line:
                    continue
                entry = json.loads(line)
                records.append(_flatten_entry(entry))

    df = pd.DataFrame(records)
    print(f"[collector] {len(df)}개 경험 로드 완료 ({len(log_files)}개 파일)")
    return df


def _flatten_entry(entry: dict) -> dict:
    """중첩된 JSON을 flat dict로 변환합니다."""
    flat = {
        "id": entry.get("id"),
        "timestamp": entry.get("timestamp", 0.0),
        "reward": entry.get("reward", 0.0),
    }

    state = entry.get("state", {})
    for k, v in state.items():
        flat[f"state_{k}"] = v

    action = entry.get("action", {})
    for k, v in action.items():
        flat[f"action_{k}"] = v

    return flat


if __name__ == "__main__":
    df = load_experiences()
    print(df.head())
    print(df.dtypes)
