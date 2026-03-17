#pragma once

#include "CoreMinimal.h"
#include "Data/ShotTypes.h"
#include "DirectorDecision.generated.h"

/**
 * AI 디렉터가 내린 카메라 결정
 *
 * ★ 이 구조체도 UE5 ↔ ML의 경계입니다 ★
 * [언리얼 담당] AIDirector에서 이 구조체를 받아 실제 카메라를 움직이면 됩니다.
 * [ML 담당]     RL의 Action(행동)에 해당합니다. 로그에 함께 저장됩니다.
 */
USTRUCT(BlueprintType)
struct CINEAIDIRECTOR_API FDirectorDecision
{
    GENERATED_BODY()

    // 선택된 샷 타입
    UPROPERTY(BlueprintReadOnly)
    ECineShotType ShotType = ECineShotType::None;

    // 카메라가 이동할 목표 위치
    UPROPERTY(BlueprintReadOnly)
    FVector TargetCameraLocation = FVector::ZeroVector;

    // 카메라가 회전할 목표 방향
    UPROPERTY(BlueprintReadOnly)
    FRotator TargetCameraRotation = FRotator::ZeroRotator;

    // 트랜지션 방식
    UPROPERTY(BlueprintReadOnly)
    ECineTransitionType TransitionType = ECineTransitionType::Blend;

    // 트랜지션 소요 시간 (초)
    UPROPERTY(BlueprintReadOnly)
    float TransitionDuration = 0.5f;

    // AI의 확신도 (0.0 ~ 1.0) - 낮으면 기본값으로 폴백할 수 있음
    UPROPERTY(BlueprintReadOnly)
    float Confidence = 0.0f;

    // API가 반환한 이유 설명 (디버깅용)
    UPROPERTY(BlueprintReadOnly)
    FString Reasoning = TEXT("");

    // 결정이 유효한지 여부
    bool IsValid() const { return ShotType != ECineShotType::None && Confidence > 0.0f; }
};
