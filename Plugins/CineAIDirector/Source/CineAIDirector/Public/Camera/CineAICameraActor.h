#pragma once

#include "CoreMinimal.h"
#include "CineCameraActor.h"
#include "Data/DirectorDecision.h"
#include "CineAICameraActor.generated.h"

class UCineAICameraComponent;

/**
 * AI 디렉터가 제어하는 시네마틱 카메라 액터
 *
 * [언리얼 담당] 레벨에 이 액터를 배치하면 됩니다.
 *               AIDirector가 자동으로 이 액터를 찾아서 제어합니다.
 *               직접 블루프린트로 상속받아 커스터마이징해도 됩니다.
 */
UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="Cine AI Camera"))
class CINEAIDIRECTOR_API ACineAICameraActor : public ACineCameraActor
{
    GENERATED_BODY()

public:
    ACineAICameraActor();

    /**
     * AI 디렉터의 결정을 받아 카메라를 이동시킵니다.
     * [언리얼 담당] 이 함수는 AIDirector가 자동으로 호출합니다. 직접 호출할 필요 없어요.
     */
    UFUNCTION(BlueprintCallable, Category="CineAI|Camera")
    void ApplyDecision(const FDirectorDecision& Decision);

    /**
     * 현재 카메라가 트랜지션 중인지 여부
     */
    UFUNCTION(BlueprintPure, Category="CineAI|Camera")
    bool IsTransitioning() const { return bIsTransitioning; }

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

private:
    // 트랜지션 진행 상태
    bool bIsTransitioning = false;
    float TransitionProgress = 0.0f;
    float TransitionDuration = 0.0f;

    // 트랜지션 시작/끝 위치
    FVector StartLocation;
    FVector TargetLocation;
    FRotator StartRotation;
    FRotator TargetRotation;

    void UpdateTransition(float DeltaTime);
};
