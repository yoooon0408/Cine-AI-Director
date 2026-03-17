// Copyright Cine-AI-Director Team. All Rights Reserved.

#include "Camera/CineAICameraActor.h"
#include "Camera/CineAICameraComponent.h"

ACineAICameraActor::ACineAICameraActor()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ACineAICameraActor::BeginPlay()
{
    Super::BeginPlay();
}

void ACineAICameraActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsTransitioning)
    {
        UpdateTransition(DeltaTime);
    }
}

void ACineAICameraActor::ApplyDecision(const FDirectorDecision& Decision)
{
    if (!Decision.IsValid()) return;

    StartLocation = GetActorLocation();
    StartRotation = GetActorRotation();
    TargetLocation = Decision.TargetCameraLocation;
    TargetRotation = Decision.TargetCameraRotation;

    if (Decision.TransitionType == ECineTransitionType::Cut)
    {
        // 즉시 이동
        SetActorLocation(TargetLocation);
        SetActorRotation(TargetRotation);
        bIsTransitioning = false;
    }
    else
    {
        // 블렌드 트랜지션 시작
        TransitionDuration = FMath::Max(Decision.TransitionDuration, 0.01f);
        TransitionProgress = 0.0f;
        bIsTransitioning = true;
    }
}

void ACineAICameraActor::UpdateTransition(float DeltaTime)
{
    TransitionProgress += DeltaTime / TransitionDuration;

    if (TransitionProgress >= 1.0f)
    {
        TransitionProgress = 1.0f;
        bIsTransitioning = false;
    }

    // EaseInOut 보간으로 부드러운 카메라 이동
    float Alpha = FMath::SmoothStep(0.0f, 1.0f, TransitionProgress);
    SetActorLocation(FMath::Lerp(StartLocation, TargetLocation, Alpha));
    SetActorRotation(FMath::Lerp(StartRotation, TargetRotation, Alpha));
}
