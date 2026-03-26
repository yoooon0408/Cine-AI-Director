// Copyright Cine-AI-Director Team. All Rights Reserved.

#include "Camera/ShotBlender.h"

UShotBlender::UShotBlender()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UShotBlender::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

float UShotBlender::ComputeAlpha(ECineTransitionType TransitionType, float RawProgress) const
{
    RawProgress = FMath::Clamp(RawProgress, 0.0f, 1.0f);

    switch (TransitionType)
    {
        case ECineTransitionType::Cut:
            return 1.0f;

        case ECineTransitionType::Blend:
            // EaseInOut - 시작과 끝이 부드럽게
            return FMath::SmoothStep(0.0f, 1.0f, RawProgress);

        case ECineTransitionType::Fade:
            // 선형 페이드
            return RawProgress;

        default:
            return RawProgress;
    }
}
