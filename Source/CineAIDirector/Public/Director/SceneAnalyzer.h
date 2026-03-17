#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/SceneContext.h"
#include "SceneAnalyzer.generated.h"

/**
 * 씬 상태를 수집/분석하는 컴포넌트
 *
 * [언리얼 담당] 이 컴포넌트가 게임 내 캐릭터를 추적해서 SceneContext를 만듭니다.
 *               AIDirector 액터에 붙어서 작동합니다.
 *
 *   주요 역할:
 *   1. 레벨 내 캐릭터 위치/상태 수집
 *   2. 현재 씬 상황 분류 (전투인지, 대화인지 등)
 *   3. SceneContext 구조체를 완성해서 반환
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
     * [언리얼 담당] 게임에 맞게 분류 로직을 수정해주세요.
     */
    UFUNCTION(BlueprintCallable, Category="CineAI|Analysis")
    ECineSceneType ClassifyScene(const TArray<FCineCharacterInfo>& Characters);

    // 분석 주기 (초) - 너무 짧으면 API 과부하
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CineAI|Settings", meta=(ClampMin="0.5"))
    float AnalysisInterval = 2.0f;

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    // 레벨 내 캐릭터 정보를 수집합니다.
    TArray<FCineCharacterInfo> GatherCharacterInfos();

    float TimeSinceLastAnalysis = 0.0f;
};
