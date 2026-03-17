#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Data/SceneContext.h"
#include "Data/DirectorDecision.h"
#include "RewardEvaluator.generated.h"

/**
 * 카메라 결정의 품질을 평가해서 리워드 신호를 생성합니다.
 *
 * [ML 담당] 이 클래스가 강화학습의 리워드 함수입니다.
 *           초기엔 촬영 원칙 기반의 규칙으로 구현하고,
 *           나중에 인간 피드백(RLHF) 방식으로 교체 가능합니다.
 *
 * [언리얼 담당] 이 파일은 건드리지 않아도 됩니다.
 */
UCLASS(BlueprintType)
class CINEAIDIRECTOR_API URewardEvaluator : public UObject
{
    GENERATED_BODY()

public:
    /**
     * 씬 컨텍스트와 결정을 바탕으로 리워드를 계산합니다.
     * 값 범위: -1.0 (최악) ~ 1.0 (최고)
     */
    UFUNCTION(BlueprintCallable, Category="CineAI|Reward")
    float EvaluateDecision(const FSceneContext& Context, const FDirectorDecision& Decision);

private:
    // 피사체 프레이밍 점수 (주인공이 화면 중앙에 있는지)
    float ScoreFraming(const FSceneContext& Context, const FDirectorDecision& Decision);

    // 샷 타입 적합성 점수 (씬 상황에 맞는 샷인지)
    float ScoreShotAppropriateness(const FSceneContext& Context, const FDirectorDecision& Decision);

    // 샷 유지 시간 점수 (너무 자주 바꾸지 않는지)
    float ScoreShotDuration(const FSceneContext& Context);
};
