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

// AI 결정이 내려졌을 때 블루프린트에서 이벤트를 받을 수 있는 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDecisionMade, const FDirectorDecision&, Decision);

/**
 * AI 카메라 디렉터의 핵심 액터
 *
 * ┌─────────────────────────────────────────────────────────────┐
 * │  [언리얼 담당] 세팅 방법                                     │
 * │                                                             │
 * │  1. 레벨에 이 액터(AIDirector)를 하나 배치하세요.            │
 * │  2. 레벨에 CineAICameraActor를 하나 배치하세요.              │
 * │  3. AIDirector의 Details > CineAI|Setup >                  │
 * │     "Target Camera" 에 CineAICameraActor를 연결하세요.      │
 * │  4. "API Key" 에 Claude API 키를 입력하세요.                │
 * │     (없어도 됩니다 — 규칙 기반 폴백으로 자동 작동합니다)     │
 * │                                                             │
 * │  그 외에는 자동으로 작동합니다!                              │
 * └─────────────────────────────────────────────────────────────┘
 *
 * [ML 담당] API 호출/파싱/샷 포지셔닝 로직은 .cpp에 있습니다.
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

    // ───────────────────────────────────────────────────────────
    // [언리얼 담당] Details 패널에서 설정하는 항목들
    // ───────────────────────────────────────────────────────────

    // 제어할 카메라 액터 (레벨에 배치된 CineAICameraActor를 여기에 연결하세요)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CineAI|Setup")
    ACineAICameraActor* TargetCamera = nullptr;

    // AI API 활성화 여부 (끄면 규칙 기반 폴백으로 작동)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CineAI|Setup")
    bool bEnableAIAPI = true;

    // 로컬 추론 서버 사용 여부 (ML 모델이 학습된 후 API 대신 사용)
    // [ML 담당] ML/src/inference/server.py 를 실행한 후 이 옵션을 켜세요.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CineAI|Setup")
    bool bUseLocalInferenceServer = false;

    // ───────────────────────────────────────────────────────────
    // [ML 담당] API 설정 항목들
    // ───────────────────────────────────────────────────────────

    // Claude AI API 엔드포인트
    UPROPERTY(EditAnywhere, Category="CineAI|API")
    FString APIEndpoint = TEXT("https://api.anthropic.com/v1/messages");

    // 로컬 추론 서버 엔드포인트 (bUseLocalInferenceServer = true 일 때 사용)
    UPROPERTY(EditAnywhere, Category="CineAI|API")
    FString LocalInferenceEndpoint = TEXT("http://127.0.0.1:8765/predict");

    // Claude API 키 (배포 시 환경변수로 분리 권장)
    UPROPERTY(EditAnywhere, Category="CineAI|API")
    FString APIKey = TEXT("");

    // 분석 주기 (초) — API 비용 절감을 위해 2~5초 권장
    UPROPERTY(EditAnywhere, Category="CineAI|API", meta=(ClampMin="1.0"))
    float DecisionInterval = 3.0f;

    // 최소 샷 유지 시간 (초) — 이보다 짧은 샷은 강제로 연장됩니다
    UPROPERTY(EditAnywhere, Category="CineAI|API", meta=(ClampMin="0.5"))
    float MinShotDuration = 2.0f;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

private:
    UPROPERTY()
    USceneAnalyzer* SceneAnalyzer = nullptr;

    UPROPERTY()
    UExperienceLogger* ExperienceLogger = nullptr;

    float TimeSinceLastDecision  = 0.0f;
    float CurrentShotDuration    = 0.0f;
    ECineShotType LastAppliedShotType = ECineShotType::None; // 마지막으로 적용된 샷 타입 추적
    FSceneContext LastContext;

    // Claude API 호출
    void RequestDecisionFromClaudeAPI(const FSceneContext& Context);

    // 로컬 추론 서버 호출
    void RequestDecisionFromLocalServer(const FSceneContext& Context);

    // API/서버 응답 처리
    void OnAPIResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess);
    void OnLocalServerResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess);

    // API 없이 씬 상황에 맞는 규칙 기반 결정
    FDirectorDecision FallbackDecision(const FSceneContext& Context);

    // SceneContext → Claude API용 JSON 직렬화 (messages 포맷)
    FString BuildClaudeRequestBody(const FSceneContext& Context);

    // SceneContext → 로컬 서버용 JSON 직렬화
    FString BuildLocalServerRequestBody(const FSceneContext& Context);

    // Claude API 응답(content[0].text) → FDirectorDecision 파싱
    FDirectorDecision ParseClaudeResponse(const FString& ResponseBody);

    // 로컬 서버 응답 → FDirectorDecision 파싱
    FDirectorDecision ParseLocalServerResponse(const FString& ResponseBody);

    // JSON 오브젝트에서 FDirectorDecision을 읽는 공통 함수
    FDirectorDecision ParseDecisionFromJsonObject(const TSharedPtr<FJsonObject>& JsonObject);

    // 결정을 카메라에 적용하고 로깅
    void ApplyAndLog(const FDirectorDecision& Decision);

    /**
     * 샷 타입 + 씬 컨텍스트 → 실제 카메라 위치/방향 계산
     *
     * [ML 담당] 이 함수가 API가 반환한 shot_type을
     *           실제 UE5 월드 좌표의 카메라 Transform으로 변환합니다.
     *           각 샷 타입의 오프셋/거리 값을 여기서 조정하세요.
     */
    void ComputeShotTransform(
        const FSceneContext& Context,
        ECineShotType ShotType,
        FVector& OutLocation,
        FRotator& OutRotation
    );

    // 씬 타입 enum을 사람이 읽을 수 있는 문자열로 변환 (프롬프트용)
    static FString SceneTypeToString(ECineSceneType SceneType);

    // 샷 타입 enum을 문자열로 변환 (프롬프트용)
    static FString ShotTypeToString(ECineShotType ShotType);
};
