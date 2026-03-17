"""
강화학습 훈련 진입점

[ML 담당] 여기서 훈련을 시작합니다.
          $ python trainer.py
"""

from pathlib import Path
from stable_baselines3 import PPO
from stable_baselines3.common.callbacks import EvalCallback, CheckpointCallback

from training.environment import CineDirectorEnv


PROCESSED_DATA_DIR = Path(__file__).parents[2] / "data" / "processed"
CHECKPOINT_DIR     = Path(__file__).parents[2] / "models" / "checkpoints"
EXPORT_DIR         = Path(__file__).parents[2] / "models" / "exports"


def train(
    total_timesteps: int = 100_000,
    learning_rate: float = 3e-4,
    n_steps: int = 2048,
):
    """PPO 기반 카메라 디렉터 에이전트 훈련"""
    CHECKPOINT_DIR.mkdir(parents=True, exist_ok=True)
    EXPORT_DIR.mkdir(parents=True, exist_ok=True)

    env     = CineDirectorEnv(dataset_path=PROCESSED_DATA_DIR)
    eval_env = CineDirectorEnv(dataset_path=PROCESSED_DATA_DIR)

    model = PPO(
        policy="MlpPolicy",
        env=env,
        learning_rate=learning_rate,
        n_steps=n_steps,
        verbose=1,
        tensorboard_log=str(CHECKPOINT_DIR / "tb_logs"),
    )

    callbacks = [
        CheckpointCallback(
            save_freq=10_000,
            save_path=str(CHECKPOINT_DIR),
            name_prefix="cine_director",
        ),
        EvalCallback(
            eval_env=eval_env,
            eval_freq=5_000,
            best_model_save_path=str(CHECKPOINT_DIR / "best"),
            verbose=1,
        ),
    ]

    print(f"[trainer] 훈련 시작 - {total_timesteps:,} 스텝")
    model.learn(total_timesteps=total_timesteps, callback=callbacks)

    # 최종 모델 저장
    model.save(str(EXPORT_DIR / "cine_director_final"))
    print(f"[trainer] 훈련 완료. 저장: {EXPORT_DIR}")

    return model


if __name__ == "__main__":
    train()
