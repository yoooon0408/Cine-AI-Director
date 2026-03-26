#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ICineCharacterInterface.generated.h"

// UInterface 선언 (UE5 내부용 — 이 클래스는 직접 수정하지 마세요)
UINTERFACE(MinimalAPI, BlueprintType, Blueprintable)
class UCineCharacterInterface : public UInterface
{
    GENERATED_BODY()
};

/**
 * CineAI 카메라 시스템에 캐릭터 상태를 노출하는 블루프린트 인터페이스
 *
 * ┌──────────────────────────────────────────────────────────────┐
 * │  [언리얼 담당] 사용법 (블루프린트 기준)                       │
 * │                                                              │
 * │  1. 캐릭터 블루프린트를 열고                                  │
 * │     상단 메뉴 > Class Settings > Implemented Interfaces 에서 │
 * │     "CineCharacterInterface" 를 추가하세요.                   │
 * │                                                              │
 * │  2. 아래 4개 함수가 My Blueprint > Interfaces 탭에 생깁니다. │
 * │     오버라이드(Override)해서 블루프린트로 구현해주세요.        │
 * │                                                              │
 * │  3. 구현하지 않아도 됩니다.                                   │
 * │     구현 안 하면 기본값으로 처리됩니다:                       │
 * │       - GetHealthNormalized → 1.0 (항상 풀피)                │
 * │       - IsSpeaking         → false                          │
 * │       - IsInCombat         → false (속도로 자동 감지)        │
 * │       - IsDead             → false                          │
 * └──────────────────────────────────────────────────────────────┘
 */
class CINEAIDIRECTOR_API ICineCharacterInterface
{
    GENERATED_BODY()

public:
    /**
     * 현재 체력을 0.0(사망) ~ 1.0(풀피) 범위로 반환합니다.
     *
     * [블루프린트 구현 예시]
     *   Return Value = CurrentHP / MaxHP
     *
     * 기본값: 1.0 (구현하지 않으면 항상 풀피로 간주)
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="CineAI|Character")
    float GetHealthNormalized() const;

    /**
     * 현재 대화 중이면 true를 반환합니다.
     *
     * [블루프린트 구현 예시]
     *   대화 시스템(예: Dialogue Manager)이 활성화됐을 때 true 반환
     *
     * 기본값: false
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="CineAI|Character")
    bool IsSpeaking() const;

    /**
     * 현재 전투 상태(공격 중, 피격 중 등)이면 true를 반환합니다.
     * 구현하지 않으면 이동 속도를 기반으로 자동으로 전투/탐험을 구분합니다.
     *
     * [블루프린트 구현 예시]
     *   bIsAttacking || bIsHit 등 전투 관련 bool 변수를 OR 연결
     *
     * 기본값: false
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="CineAI|Character")
    bool IsInCombat() const;

    /**
     * 사망 상태이면 true를 반환합니다.
     *
     * [블루프린트 구현 예시]
     *   Return Value = (CurrentHP <= 0)
     *
     * 기본값: false
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="CineAI|Character")
    bool IsDead() const;
};
