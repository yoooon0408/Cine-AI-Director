#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/SceneContext.h"
#include "Data/DirectorDecision.h"
#include "Http.h"
#include "AIDirector.generated.h"

class USceneAnalyzer;
class ACineAICameraActor;
class UExperienceLogger;

// AI API 응답이 왔을 때 호출되는 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDecisionMade, const FDirectorDecision&, Decision);

/**
 * AI 카메라 디렉터의 핵심 액터
 *
 * [언리얼 담당] 레벨에 이 액터 하나를 배치하면 자동으로 작동합니다.
 *               Details 패널에서 카메라 액터와 API 키를 연결해주세요.
 *
 * [ML 담당] 외부 AI API 호출 로직은 여기 있습니다.
 *           API 엔드포인트와 프롬프트 설계는 이 클래스에서 담당합니다.
 */
UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="Cine AI Director"))
class CINEAIDIRECTOR_API AAIDirector : public AActor
{
    GENERATED_BODY()

public:
    AAIDirector();

    // 결정이 내려졌을 때 블루프린트에서 이벤트를 받을 수 있습니다.
    UPROPERTY(BlueprintAssignable, Category="CineAI|Events")
    FOnDecisionMade OnDecisionMade;

    // ---------------------------------------------------------------
    // [언리얼 담당] Details 패널에서 설정하는 항목들
    // ---------------------------------------------------------------

    // 제어할 카메라 액터 (레벨에 배치된 CineAICameraActor를 연결하세요)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CineAI|Setup")
    ACineAICameraActor* TargetCamera = nullptr;

    // AI API 활성화 여부 (끄면 폴백 로직으로 동작)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CineAI|Setup")
    bool bEnableAIAPI = true;

    // ---------------------------------------------------------------
    // [ML 담당] API 설정 항목들
    // ---------------------------------------------------------------

    // AI API 엔드포인트 URL
    UPROPERTY(EditAnywhere, Category="CineAI|API")
    FString APIEndpoint = TEXT("https://api.anthropic.com/v1/messages");

    // API 키 (실제 배포 시 환경변수나 설정 파일로 분리하세요)
    UPROPERTY(EditAnywhere, Category="CineAI|API")
    FString APIKey = TEXT("");

    // 분석 주기 (초)
    UPROPERTY(EditAnywhere, Category="CineAI|API", meta=(ClampMin="1.0"))
    float DecisionInterval = 3.0f;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

private:
    UPROPERTY()
    USceneAnalyzer* SceneAnalyzer = nullptr;

    UPROPERTY()
    UExperienceLogger* ExperienceLogger = nullptr;

    float TimeSinceLastDecision = 0.0f;
    FSceneContext LastContext;

    // AI API에 씬 컨텍스트를 전송하고 샷 결정을 요청합니다.
    void RequestDecisionFromAPI(const FSceneContext& Context);

    // API 응답 처리
    void OnAPIResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess);

    // API 실패 시 규칙 기반 폴백 결정
    FDirectorDecision FallbackDecision(const FSceneContext& Context);

    // SceneContext -> JSON 직렬화
    FString SerializeContextToJSON(const FSceneContext& Context);

    // JSON 응답 -> DirectorDecision 파싱
    FDirectorDecision ParseDecisionFromJSON(const FString& JSONString);
};
