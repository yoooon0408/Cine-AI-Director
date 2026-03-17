// Copyright Cine-AI-Director Team. All Rights Reserved.

#include "Logging/RewardEvaluator.h"

float URewardEvaluator::EvaluateDecision(const FSceneContext& Context, const FDirectorDecision& Decision)
{
    // 각 항목을 가중 평균으로 합산
    float FramingScore       = ScoreFraming(Context, Decision)           * 0.4f;
    float AppropriateScore   = ScoreShotAppropriateness(Context, Decision) * 0.4f;
    float DurationScore      = ScoreShotDuration(Context)                * 0.2f;

    float TotalReward = FramingScore + AppropriateScore + DurationScore;
    return FMath::Clamp(TotalReward, -1.0f, 1.0f);
}

float URewardEvaluator::ScoreFraming(const FSceneContext& Context, const FDirectorDecision& Decision)
{
    // TODO: [ML 담당] 카메라 FOV와 피사체 위치를 비교해서 프레이밍 점수 계산
    // 현재는 플레이스홀더
    return 0.5f;
}

float URewardEvaluator::ScoreShotAppropriateness(const FSceneContext& Context, const FDirectorDecision& Decision)
{
    // 씬 상황별 권장 샷 타입 매핑
    // [ML 담당] 이 매핑이 초기 리워드 설계의 핵심입니다. 필요에 따라 조정하세요.
    switch (Context.SceneType)
    {
        case ECineSceneType::Dialogue:
            // 대화 씬 - 클로즈업/미디엄/오버숄더 선호
            if (Decision.ShotType == ECineShotType::CloseUp
             || Decision.ShotType == ECineShotType::MediumCloseUp
             || Decision.ShotType == ECineShotType::OverShoulder)
                return 1.0f;
            if (Decision.ShotType == ECineShotType::Wide)
                return -0.3f;
            return 0.2f;

        case ECineSceneType::Combat:
            // 전투 씬 - 와이드/미디엄 선호
            if (Decision.ShotType == ECineShotType::Wide
             || Decision.ShotType == ECineShotType::Medium)
                return 1.0f;
            if (Decision.ShotType == ECineShotType::ExtremeCloseUp)
                return -0.5f;
            return 0.3f;

        case ECineSceneType::Exploration:
            // 탐험 - 와이드/버즈아이 선호
            if (Decision.ShotType == ECineShotType::Wide
             || Decision.ShotType == ECineShotType::ExtremeWide
             || Decision.ShotType == ECineShotType::BirdsEye)
                return 1.0f;
            return 0.0f;

        default:
            return 0.0f;
    }
}

float URewardEvaluator::ScoreShotDuration(const FSceneContext& Context)
{
    // 샷이 너무 짧으면 페널티, 적당하면 보너스
    float Duration = Context.CurrentShotDuration;
    if (Duration < 1.5f)  return -0.5f;  // 너무 짧음
    if (Duration < 3.0f)  return  0.5f;  // 적당
    if (Duration < 8.0f)  return  1.0f;  // 이상적
    if (Duration < 15.0f) return  0.3f;  // 약간 긺
    return -0.3f;                         // 너무 긺
}
