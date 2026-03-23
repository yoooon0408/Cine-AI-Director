// Copyright Cine-AI-Director Team. All Rights Reserved.

#include "Logging/RewardEvaluator.h"

float URewardEvaluator::EvaluateDecision(const FSceneContext& Context, const FDirectorDecision& Decision)
{
    float FramingScore     = ScoreFraming(Context, Decision)              * 0.4f;
    float AppropriateScore = ScoreShotAppropriateness(Context, Decision)  * 0.4f;
    float DurationScore    = ScoreShotDuration(Context)                   * 0.2f;

    float TotalReward = FramingScore + AppropriateScore + DurationScore;
    return FMath::Clamp(TotalReward, -1.0f, 1.0f);
}

float URewardEvaluator::ScoreFraming(const FSceneContext& Context, const FDirectorDecision& Decision)
{
    // 주인공 캐릭터를 찾습니다.
    const FCineCharacterInfo* FocusChar = nullptr;
    for (const FCineCharacterInfo& C : Context.Characters)
    {
        if (C.bIsPlayer) { FocusChar = &C; break; }
    }
    if (!FocusChar && Context.Characters.Num() > 0)
        FocusChar = &Context.Characters[0];

    if (!FocusChar)
    {
        return 0.5f; // 캐릭터가 없으면 중간 점수
    }

    // 카메라와 캐릭터 사이의 거리를 샷 타입 기준 이상 거리와 비교합니다.
    float DistToSubject = FVector::Dist(Context.CameraLocation, FocusChar->Location);

    // 샷 타입별 이상적인 거리 범위 (최소, 최대) — cm 단위
    float IdealMin = 0.0f;
    float IdealMax = 0.0f;

    switch (Decision.ShotType)
    {
        case ECineShotType::ExtremeWide:    IdealMin = 1000.0f; IdealMax = 2500.0f; break;
        case ECineShotType::Wide:           IdealMin =  500.0f; IdealMax = 1000.0f; break;
        case ECineShotType::Medium:         IdealMin =  200.0f; IdealMax =  400.0f; break;
        case ECineShotType::MediumCloseUp:  IdealMin =  120.0f; IdealMax =  220.0f; break;
        case ECineShotType::CloseUp:        IdealMin =   60.0f; IdealMax =  130.0f; break;
        case ECineShotType::ExtremeCloseUp: IdealMin =   20.0f; IdealMax =   70.0f; break;
        case ECineShotType::OverShoulder:   IdealMin =   80.0f; IdealMax =  200.0f; break;
        case ECineShotType::TwoShot:        IdealMin =  200.0f; IdealMax =  500.0f; break;
        case ECineShotType::POV:            IdealMin =    0.0f; IdealMax =   50.0f; break;
        case ECineShotType::BirdsEye:       IdealMin =  800.0f; IdealMax = 2000.0f; break;
        case ECineShotType::LowAngle:       IdealMin =  100.0f; IdealMax =  300.0f; break;
        case ECineShotType::Dutch:          IdealMin =  150.0f; IdealMax =  400.0f; break;
        default:                            return 0.5f;
    }

    // 이상적인 거리 범위 안이면 1.0, 벗어날수록 0으로 감소
    if (DistToSubject >= IdealMin && DistToSubject <= IdealMax)
    {
        return 1.0f;
    }

    float Overshoot = 0.0f;
    if (DistToSubject < IdealMin)
        Overshoot = (IdealMin - DistToSubject) / FMath::Max(IdealMin, 1.0f);
    else
        Overshoot = (DistToSubject - IdealMax) / FMath::Max(IdealMax, 1.0f);

    // 거리 오차가 50% 이상이면 페널티
    float Score = FMath::Clamp(1.0f - Overshoot * 2.0f, -0.5f, 1.0f);
    return Score;
}

float URewardEvaluator::ScoreShotAppropriateness(const FSceneContext& Context, const FDirectorDecision& Decision)
{
    // 씬 상황별 권장 샷 타입 매핑
    switch (Context.SceneType)
    {
        case ECineSceneType::Dialogue:
            if (Decision.ShotType == ECineShotType::CloseUp
             || Decision.ShotType == ECineShotType::MediumCloseUp
             || Decision.ShotType == ECineShotType::OverShoulder
             || Decision.ShotType == ECineShotType::TwoShot)
                return 1.0f;
            if (Decision.ShotType == ECineShotType::ExtremeWide
             || Decision.ShotType == ECineShotType::BirdsEye)
                return -0.5f;
            return 0.2f;

        case ECineSceneType::Combat:
            if (Decision.ShotType == ECineShotType::Wide
             || Decision.ShotType == ECineShotType::Medium)
                return 1.0f;
            if (Decision.ShotType == ECineShotType::LowAngle
             || Decision.ShotType == ECineShotType::Dutch)
                return 0.5f; // 전투 긴장감 연출로 나쁘지 않음
            if (Decision.ShotType == ECineShotType::ExtremeCloseUp)
                return -0.5f; // 전투 상황 파악 어려움
            return 0.3f;

        case ECineSceneType::Exploration:
            if (Decision.ShotType == ECineShotType::Wide
             || Decision.ShotType == ECineShotType::ExtremeWide
             || Decision.ShotType == ECineShotType::BirdsEye)
                return 1.0f;
            if (Decision.ShotType == ECineShotType::Medium)
                return 0.4f;
            return 0.0f;

        case ECineSceneType::Death:
            if (Decision.ShotType == ECineShotType::CloseUp
             || Decision.ShotType == ECineShotType::LowAngle
             || Decision.ShotType == ECineShotType::MediumCloseUp)
                return 1.0f;
            return 0.2f;

        case ECineSceneType::Victory:
            if (Decision.ShotType == ECineShotType::Wide
             || Decision.ShotType == ECineShotType::BirdsEye
             || Decision.ShotType == ECineShotType::ExtremeWide)
                return 1.0f;
            return 0.3f;

        case ECineSceneType::Cutscene:
            // 컷씬은 어떤 샷이든 감독의 의도를 따름 — 중간 점수
            return 0.5f;

        default:
            return 0.0f;
    }
}

float URewardEvaluator::ScoreShotDuration(const FSceneContext& Context)
{
    float Duration = Context.CurrentShotDuration;

    if (Duration < 1.5f)   return -0.5f;  // 너무 빠른 전환 — 불안정
    if (Duration < 3.0f)   return  0.5f;  // 짧지만 허용 가능
    if (Duration < 8.0f)   return  1.0f;  // 이상적인 샷 유지 시간
    if (Duration < 15.0f)  return  0.3f;  // 약간 길지만 OK
    return -0.3f;                         // 너무 오래 유지 — 지루함
}
