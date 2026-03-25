#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/ShotTypes.h"
#include "ShotBlender.generated.h"

/**
 * 샷 간 블렌딩을 담당하는 컴포넌트
 *
 * [언리얼 담당] 트랜지션 커브나 이징 함수를 여기서 커스터마이징할 수 있어요.
 *               기본 블렌드 외에 페이드, 줌 등 효과를 추가하고 싶다면 이 클래스를 수정하세요.
 */
UCLASS(ClassGroup=(CineAI), meta=(BlueprintSpawnableComponent))
class CINEAIDIRECTOR_API UShotBlender : public UActorComponent
{
    GENERATED_BODY()

public:
    UShotBlender();

    // 트랜지션 진행률 (0.0 ~ 1.0)
    UFUNCTION(BlueprintPure, Category="CineAI|Blend")
    float GetBlendAlpha() const { return BlendAlpha; }

    // 특정 트랜지션 타입에 맞는 알파값 계산
    float ComputeAlpha(ECineTransitionType TransitionType, float RawProgress) const;

protected:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    float BlendAlpha = 0.0f;
};
