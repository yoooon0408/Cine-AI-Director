"""
학습된 PPO 모델을 로컬 HTTP 서버로 서빙합니다.

[ML 담당] 사용법:
    # 1. 먼저 모델 학습
    python run_pipeline.py --mode synthetic --timesteps 200000

    # 2. 서버 실행
    python -m inference.server

    # 3. UE5에서 AIDirector > bUseLocalInferenceServer = true 로 설정
    #    LocalInferenceEndpoint = "http://127.0.0.1:8765/predict"

포트: 8765 (UE5 AIDirector의 LocalInferenceEndpoint 기본값과 일치)
"""

import sys
import json
import argparse
from pathlib import Path

# ML/src 디렉터리가 sys.path에 없으면 추가
_SRC_DIR = Path(__file__).parents[1]
if str(_SRC_DIR) not in sys.path:
    sys.path.insert(0, str(_SRC_DIR))

try:
    from fastapi import FastAPI, HTTPException
    from fastapi.responses import JSONResponse
    import uvicorn
    from pydantic import BaseModel
except ImportError:
    print(
        "[server] FastAPI / uvicorn이 설치되지 않았습니다.\n"
        "  실행: pip install fastapi uvicorn"
    )
    sys.exit(1)

from inference.predictor import CineDirectorPredictor


# ─────────────────────────────────────────────────────────────────────
# FastAPI 앱
# ─────────────────────────────────────────────────────────────────────

app = FastAPI(
    title="Cine AI Director - Local Inference Server",
    description="학습된 PPO 카메라 디렉터 모델을 UE5에 서빙합니다.",
    version="1.0.0",
)

predictor = CineDirectorPredictor()


class SceneStateRequest(BaseModel):
    """UE5 AIDirector가 보내는 씬 상태 (AIDirector.cpp BuildLocalServerRequestBody 와 동일)"""
    scene_type:      int   = 0
    character_count: int   = 1
    current_shot:    int   = 0
    shot_duration:   float = 0.0
    cam_x:           float = 0.0
    cam_y:           float = 0.0
    cam_z:           float = 150.0


class ShotDecisionResponse(BaseModel):
    """UE5 AIDirector가 파싱하는 결정 (AIDirector.cpp ParseDecisionFromJsonObject 와 동일)"""
    shot_type:           int
    transition_type:     int
    transition_duration: float
    confidence:          float
    reasoning:           str


# ─────────────────────────────────────────────────────────────────────
# 엔드포인트
# ─────────────────────────────────────────────────────────────────────

@app.on_event("startup")
async def startup_event():
    """서버 시작 시 모델 로드"""
    ok = predictor.load()
    if ok:
        print("[server] 모델 로드 완료 - PPO 추론 모드")
    else:
        print("[server] 모델 없음 - 규칙 기반 폴백 모드로 작동합니다")


@app.get("/health")
async def health():
    """서버 상태 확인 (UE5에서 먼저 호출해도 됨)"""
    return {
        "status": "ok",
        "model_loaded": predictor.is_loaded,
    }


@app.post("/predict", response_model=ShotDecisionResponse)
async def predict(request: SceneStateRequest):
    """
    씬 상태를 받아 카메라 샷 결정을 반환합니다.

    UE5의 AIDirector가 DecisionInterval마다 이 엔드포인트를 호출합니다.
    """
    try:
        state_dict = request.model_dump()
        result = predictor.predict(state_dict)
        return JSONResponse(content=result)
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Prediction error: {str(e)}")


@app.get("/reload")
async def reload_model():
    """모델을 다시 로드합니다. 새 모델로 재학습 후 재시작 없이 반영할 때 사용."""
    ok = predictor.load()
    return {"success": ok, "model_loaded": predictor.is_loaded}


# ─────────────────────────────────────────────────────────────────────
# 메인
# ─────────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(description="Cine AI Director Local Inference Server")
    parser.add_argument("--host", default="127.0.0.1", help="바인딩 주소 (기본: 127.0.0.1)")
    parser.add_argument("--port", type=int, default=8765,  help="포트 번호 (기본: 8765)")
    parser.add_argument(
        "--model",
        default=None,
        help="모델 파일 경로 (.zip). 기본값: models/exports/cine_director_final.zip",
    )
    args = parser.parse_args()

    if args.model:
        predictor._model_path = Path(args.model)

    print(f"[server] 서버 시작: http://{args.host}:{args.port}")
    print(f"[server] UE5 AIDirector 설정:")
    print(f"  bUseLocalInferenceServer = true")
    print(f"  LocalInferenceEndpoint   = http://{args.host}:{args.port}/predict")

    uvicorn.run(app, host=args.host, port=args.port, log_level="info")


if __name__ == "__main__":
    main()
