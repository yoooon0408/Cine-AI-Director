// Copyright Cine-AI-Director Team. All Rights Reserved.

#include "Director/AIDirector.h"
#include "Director/SceneAnalyzer.h"
#include "Camera/CineAICameraActor.h"
#include "Logging/ExperienceLogger.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonReader.h"
#include "JsonObjectConverter.h"

AAIDirector::AAIDirector()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneAnalyzer = CreateDefaultSubobject<USceneAnalyzer>(TEXT("SceneAnalyzer"));
    ExperienceLogger = CreateDefaultSubobject<UExperienceLogger>(TEXT("ExperienceLogger"));
}

void AAIDirector::BeginPlay()
{
    Super::BeginPlay();

    if (APIKey.IsEmpty() && bEnableAIAPI)
    {
        UE_LOG(LogTemp, Warning, TEXT("[AIDirector] API 키가 설정되지 않았습니다. 폴백 모드로 동작합니다."));
        bEnableAIAPI = false;
    }
}

void AAIDirector::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    TimeSinceLastDecision += DeltaTime;

    if (TimeSinceLastDecision >= DecisionInterval)
    {
        TimeSinceLastDecision = 0.0f;

        // 씬 상태 수집
        FSceneContext Context = SceneAnalyzer->CaptureContext();
        if (TargetCamera)
        {
            Context.CameraLocation = TargetCamera->GetActorLocation();
            Context.CameraRotation = TargetCamera->GetActorRotation();
        }
        LastContext = Context;

        if (bEnableAIAPI && !APIKey.IsEmpty())
        {
            RequestDecisionFromAPI(Context);
        }
        else
        {
            FDirectorDecision Decision = FallbackDecision(Context);
            if (TargetCamera) TargetCamera->ApplyDecision(Decision);
            ExperienceLogger->LogExperience(Context, Decision, 0.0f);
            OnDecisionMade.Broadcast(Decision);
        }
    }
}

void AAIDirector::RequestDecisionFromAPI(const FSceneContext& Context)
{
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(APIEndpoint);
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetHeader(TEXT("x-api-key"), APIKey);
    Request->SetHeader(TEXT("anthropic-version"), TEXT("2023-06-01"));

    FString Body = SerializeContextToJSON(Context);
    Request->SetContentAsString(Body);

    Request->OnProcessRequestComplete().BindUObject(this, &AAIDirector::OnAPIResponse);
    Request->ProcessRequest();
}

void AAIDirector::OnAPIResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
{
    if (!bSuccess || !Response.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[AIDirector] API 호출 실패 - 폴백 결정 사용"));
        FDirectorDecision Fallback = FallbackDecision(LastContext);
        if (TargetCamera) TargetCamera->ApplyDecision(Fallback);
        ExperienceLogger->LogExperience(LastContext, Fallback, 0.0f);
        return;
    }

    FDirectorDecision Decision = ParseDecisionFromJSON(Response->GetContentAsString());
    if (!Decision.IsValid())
    {
        Decision = FallbackDecision(LastContext);
    }

    if (TargetCamera) TargetCamera->ApplyDecision(Decision);

    // ★ ML 데이터 로깅 - 리워드는 나중에 RewardEvaluator가 업데이트합니다.
    ExperienceLogger->LogExperience(LastContext, Decision, 0.0f);

    OnDecisionMade.Broadcast(Decision);
}

FDirectorDecision AAIDirector::FallbackDecision(const FSceneContext& Context)
{
    // TODO: [ML 담당] 규칙 기반 폴백 로직을 여기에 구현하세요.
    // API 없이도 기본 작동이 가능하도록 합니다.
    FDirectorDecision Decision;
    Decision.ShotType = ECineShotType::Medium;
    Decision.TransitionType = ECineTransitionType::Blend;
    Decision.TransitionDuration = 1.0f;
    Decision.Confidence = 0.3f;
    Decision.Reasoning = TEXT("폴백 - 기본 미디엄 샷");
    return Decision;
}

FString AAIDirector::SerializeContextToJSON(const FSceneContext& Context)
{
    // TODO: [ML 담당] 프롬프트 엔지니어링 - API에 전달할 JSON 포맷을 여기서 설계하세요.
    TSharedPtr<FJsonObject> Root = MakeShareable(new FJsonObject);
    Root->SetStringField(TEXT("model"), TEXT("claude-opus-4-6"));
    Root->SetNumberField(TEXT("scene_type"), (int32)Context.SceneType);
    Root->SetNumberField(TEXT("character_count"), Context.Characters.Num());
    Root->SetNumberField(TEXT("current_shot"), (int32)Context.CurrentShotType);
    Root->SetNumberField(TEXT("shot_duration"), Context.CurrentShotDuration);

    FString Output;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
    FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
    return Output;
}

FDirectorDecision AAIDirector::ParseDecisionFromJSON(const FString& JSONString)
{
    // TODO: [ML 담당] API 응답 파싱 로직을 구현하세요.
    FDirectorDecision Decision;

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JSONString);
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[AIDirector] JSON 파싱 실패"));
        return Decision;
    }

    // 예시 파싱 - 실제 API 응답 포맷에 맞게 수정 필요
    int32 ShotTypeInt = 0;
    if (JsonObject->TryGetNumberField(TEXT("shot_type"), ShotTypeInt))
    {
        Decision.ShotType = (ECineShotType)ShotTypeInt;
    }

    double Confidence = 0.0;
    JsonObject->TryGetNumberField(TEXT("confidence"), Confidence);
    Decision.Confidence = (float)Confidence;

    JsonObject->TryGetStringField(TEXT("reasoning"), Decision.Reasoning);

    return Decision;
}
