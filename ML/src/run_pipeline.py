"""
전체 ML 파이프라인 실행 스크립트

[ML 담당] 데이터 수집 → 전처리 → 학습을 한 번에 실행합니다.

Usage:
    # UE5 데이터 없이 즉시 학습 (synthetic 모드)
    python run_pipeline.py

    # UE5 로그를 전처리한 뒤 학습 (dataset 모드)
    python run_pipeline.py --mode dataset

    # 스텝 수 지정
    python run_pipeline.py --timesteps 200000
"""

import argparse
import sys
from pathlib import Path

# ML/src 하위 모듈 임포트를 위해 현재 디렉터리를 path에 추가
sys.path.insert(0, str(Path(__file__).parent))

from training.trainer import train


def run_dataset_pipeline() -> None:
    """UE5 로그 → 전처리 → .npy 저장"""
    from data_pipeline.collector import load_experiences
    from data_pipeline.preprocessor import preprocess, save_processed

    raw_log_dir    = Path(__file__).parents[1] / "Saved" / "CineAILogs"
    processed_dir  = Path(__file__).parents[0].parent / "data" / "processed"

    print("[pipeline] UE5 로그 로드 중...")
    df = load_experiences(raw_log_dir)

    print("[pipeline] 전처리 중...")
    train_data, val_data = preprocess(df)
    save_processed(train_data, val_data, processed_dir)


def main():
    parser = argparse.ArgumentParser(description="Cine AI Director ML Pipeline")
    parser.add_argument(
        "--mode",
        default="synthetic",
        choices=["synthetic", "dataset"],
        help="synthetic: 랜덤 환경으로 즉시 학습 / dataset: UE5 로그 데이터 사용",
    )
    parser.add_argument("--timesteps", type=int,   default=100_000)
    parser.add_argument("--lr",        type=float, default=3e-4)
    parser.add_argument("--n_steps",   type=int,   default=2048)
    args = parser.parse_args()

    if args.mode == "dataset":
        run_dataset_pipeline()

    print(f"[pipeline] 훈련 시작 (mode={args.mode}, timesteps={args.timesteps:,})")
    train(
        mode=args.mode,
        total_timesteps=args.timesteps,
        learning_rate=args.lr,
        n_steps=args.n_steps,
    )


if __name__ == "__main__":
    main()
