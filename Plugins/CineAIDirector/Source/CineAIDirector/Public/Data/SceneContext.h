#pragma once

#include "CoreMinimal.h"
#include "Data/ShotTypes.h"
#include "SceneContext.generated.h"

/**
 * 씬 내 개별 캐릭터 정보
 * [언리얼 담당] SceneAnalyzer.cpp에서 이 구조체를 채워줍니다.
 */
USTRUCT(BlueprintType)
struct CINEAIDIRECTOR_API FCineCharacterInfo
{
    GENERATED_BODY()

    // 월드 기준 위치
    UPROPERTY(BlueprintReadOnly)
    FVector Location = FVector::ZeroVector;

    // 이동 방향 및 속도
    UPROPERTY(BlueprintReadOnly)
    FVector Velocity = FVector::ZeroVector;

    // 바라보는 방향
    UPROPERTY(BlueprintReadOnly)
    FRotator Rotation = FRotator::ZeroRotator;

    // 현재 체력 (0.0 ~ 1.0 정규화)
    UPROPERTY(BlueprintReadOnly)
    float HealthNormalized = 1.0f;

    // 이 캐릭터가 현재 말하고 있는지
    UPROPERTY(BlueprintReadOnly)
    bool bIsSpeaking = false;

    // 이 캐릭터가 현재 전투 상태인지 (ICineCharacterInterface 구현 시 채워짐)
    UPROPERTY(BlueprintReadOnly)
    bool bIsInCombat = false;

    // 이 캐릭터가 사망 상태인지 (ICineCharacterInterface 구현 시 채워짐)
    UPROPERTY(BlueprintReadOnly)
    bool bIsDead = false;

    // 이 캐릭터가 주인공인지
    UPROPERTY(BlueprintReadOnly)
    bool bIsPlayer = false;
};

/**
 * AI 디렉터에게 전달되는 씬 전체 상태 스냅샷
 *
 * ★ 이 구조체가 UE5 ↔ ML의 핵심 경계입니다 ★
 * [언리얼 담당] SceneAnalyzer가 매 틱(또는 이벤트마다) 이 구조체를 만들어 AIDirector에 넘깁니다.
 * [ML 담당]     이 구조체가 RL의 State(상태)가 됩니다. 필드 추가 시 함께 협의하세요.
 */
USTRUCT(BlueprintType)
struct CINEAIDIRECTOR_API FSceneContext
{
    GENERATED_BODY()

    // 씬에 등장하는 캐릭터 목록 (플레이어 포함)
    UPROPERTY(BlueprintReadOnly)
    TArray<FCineCharacterInfo> Characters;

    // 현재 씬 상황 분류
    UPROPERTY(BlueprintReadOnly)
    ECineSceneType SceneType = ECineSceneType::Unknown;

    // 현재 카메라 위치
    UPROPERTY(BlueprintReadOnly)
    FVector CameraLocation = FVector::ZeroVector;

    // 현재 카메라 회전
    UPROPERTY(BlueprintReadOnly)
    FRotator CameraRotation = FRotator::ZeroRotator;

    // 현재 적용된 샷 타입
    UPROPERTY(BlueprintReadOnly)
    ECineShotType CurrentShotType = ECineShotType::None;

    // 현재 샷이 유지된 시간 (초)
    UPROPERTY(BlueprintReadOnly)
    float CurrentShotDuration = 0.0f;

    // 게임 내 시각 (초)
    UPROPERTY(BlueprintReadOnly)
    float GameTime = 0.0f;
};
