#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/SceneContext.h"
#include "SceneAnalyzer.generated.h"

/**
 * 씬 상태를 수집/분석하는 컴포넌트
 *
 * [언리얼 담당] 이 컴포넌트가 게임 내 캐릭터를 추적해서 SceneContext를 만듭니다.
 *               AIDirector 액터에 붙어서 자동으로 작동합니다.
 *
 *   주요 역할:
 *   1. 레벨 내 캐릭터 위치/상태 수집 (ICineCharacterInterface 연동)
 *   2. 현재 씬 상황 분류 (사망 > 전투 > 대화 > 탐험 우선순위)
 *   3. SceneContext 구조체를 완성해서 AIDirector에 반환
 *
 * [언리얼 담당] 씬 분류를 수동으로 덮어쓰고 싶으면:
 *   Details 패널 > CineAI|Override 에서 bOverrideSceneType을 체크하고
 *   원하는 OverrideSceneType을 선택하세요.
 */
UCLASS(ClassGroup=(CineAI), meta=(BlueprintSpawnableComponent))
class CINEAIDIRECTOR_API USceneAnalyzer : public UActorComponent
{
    GENERATED_BODY()

public:
    USceneAnalyzer();

    /**
     * 현재 씬 상태 스냅샷을 반환합니다.
     * AIDirector가 매 분석 주기마다 이 함수를 호출합니다.
     */
    UFUNCTION(BlueprintCallable, Category="CineAI|Analysis")
    FSceneContext CaptureContext();

    /**
     * 씬 상황을 자동으로 분류합니다.
     * 우선순위: 사망 > 전투 > 대화 > 탐험 > 알 수 없음
     */
    UFUNCTION(BlueprintCallable, Category="CineAI|Analysis")
    ECineSceneType ClassifyScene(const TArray<FCineCharacterInfo>& Characters);

    // ───────────────────────────────────────────────────────────
    // [언리얼 담당] Details 패널에서 조정하는 설정들
    // ───────────────────────────────────────────────────────────

    // 분석 주기 (초) — 너무 짧으면 API 과부하
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CineAI|Settings", meta=(ClampMin="0.5"))
    float AnalysisInterval = 2.0f;

    // 전투 판단 기준 이동 속도 (cm/s) — 이 속도 이상이면 전투/탐험으로 분류
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CineAI|Settings", meta=(ClampMin="50.0"))
    float CombatVelocityThreshold = 200.0f;

    // ───────────────────────────────────────────────────────────
    // [언리얼 담당] 씬 타입 수동 오버라이드
    // 특정 이벤트(보스전, 컷씬 등)에서 씬 타입을 직접 지정하고 싶을 때 사용하세요.
    // ───────────────────────────────────────────────────────────

    // 체크하면 아래 OverrideSceneType이 자동 분류를 무시하고 강제 적용됩니다.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CineAI|Override")
    bool bOverrideSceneType = false;

    // bOverrideSceneType이 true일 때 사용할 씬 타입
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CineAI|Override",
              meta=(EditCondition="bOverrideSceneType"))
    ECineSceneType OverrideSceneType = ECineSceneType::Unknown;

    /**
     * 블루프린트에서 씬 타입을 일시적으로 오버라이드합니다.
     * 예: 컷씬 시작 시 BeginPlay/이벤트에서 호출
     */
    UFUNCTION(BlueprintCallable, Category="CineAI|Override")
    void SetSceneTypeOverride(ECineSceneType SceneType);

    /**
     * 씬 타입 오버라이드를 해제하고 자동 분류로 복귀합니다.
     */
    UFUNCTION(BlueprintCallable, Category="CineAI|Override")
    void ClearSceneTypeOverride();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    // 레벨 내 캐릭터 정보를 수집합니다 (ICineCharacterInterface 연동 포함).
    TArray<FCineCharacterInfo> GatherCharacterInfos();

    float TimeSinceLastAnalysis = 0.0f;
};
