// Copyright Cine-AI-Director Team. All Rights Reserved.

#include "Director/AIDirector.h"
#include "Director/SceneAnalyzer.h"
#include "Camera/CineAICameraActor.h"
#include "Logging/ExperienceLogger.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonReader.h"
#include "JsonObjectConverter.h"
#include "Math/UnrealMathUtility.h"

// ─────────────────────────────────────────────────────────────────────
// 초기화
// ─────────────────────────────────────────────────────────────────────

AAIDirector::AAIDirector()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneAnalyzer    = CreateDefaultSubobject<USceneAnalyzer>(TEXT("SceneAnalyzer"));
    ExperienceLogger = CreateDefaultSubobject<UExperienceLogger>(TEXT("ExperienceLogger"));
}

void AAIDirector::BeginPlay()
{
    Super::BeginPlay();

    if (APIKey.IsEmpty() && bEnableAIAPI && !bUseLocalInferenceServer)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[AIDirector] API 키가 없습니다. 규칙 기반 폴백 모드로 동작합니다."));
        bEnableAIAPI = false;
    }

    if (bUseLocalInferenceServer)
    {
        UE_LOG(LogTemp, Log,
            TEXT("[AIDirector] 로컬 추론 서버 모드: %s"), *LocalInferenceEndpoint);
    }
}

// ─────────────────────────────────────────────────────────────────────
// 매 틱 — 분석 주기마다 씬 캡처 후 결정 요청
// ─────────────────────────────────────────────────────────────────────

void AAIDirector::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    TimeSinceLastDecision += DeltaTime;
    CurrentShotDuration   += DeltaTime;

    if (TimeSinceLastDecision < DecisionInterval) return;
    if (CurrentShotDuration   < MinShotDuration)  return; // 최소 샷 유지 시간 준수

    TimeSinceLastDecision = 0.0f;

    // 씬 상태 수집
    FSceneContext Context = SceneAnalyzer->CaptureContext();
    Context.CurrentShotType     = LastAppliedShotType;  // 직전 샷 타입을 AI에게 알려줌
    Context.CurrentShotDuration = CurrentShotDuration;
    if (TargetCamera)
    {
        Context.CameraLocation = TargetCamera->GetActorLocation();
        Context.CameraRotation = TargetCamera->GetActorRotation();
    }
    LastContext = Context;

    if (bUseLocalInferenceServer)
    {
        RequestDecisionFromLocalServer(Context);
    }
    else if (bEnableAIAPI && !APIKey.IsEmpty())
    {
        RequestDecisionFromClaudeAPI(Context);
    }
    else
    {
        ApplyAndLog(FallbackDecision(Context));
    }
}

// ─────────────────────────────────────────────────────────────────────
// Claude API 호출
// ─────────────────────────────────────────────────────────────────────

void AAIDirector::RequestDecisionFromClaudeAPI(const FSceneContext& Context)
{
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(APIEndpoint);
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"),      TEXT("application/json"));
    Request->SetHeader(TEXT("x-api-key"),         APIKey);
    Request->SetHeader(TEXT("anthropic-version"), TEXT("2023-06-01"));
    Request->SetContentAsString(BuildClaudeRequestBody(Context));
    Request->OnProcessRequestComplete().BindUObject(this, &AAIDirector::OnAPIResponse);
    Request->ProcessRequest();
}

void AAIDirector::OnAPIResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
{
    if (!bSuccess || !Response.IsValid() || Response->GetResponseCode() != 200)
    {
        int32 Code = Response.IsValid() ? Response->GetResponseCode() : 0;
        UE_LOG(LogTemp, Warning,
            TEXT("[AIDirector] Claude API 실패 (코드 %d) — 폴백 결정 사용"), Code);
        ApplyAndLog(FallbackDecision(LastContext));
        return;
    }

    FDirectorDecision Decision = ParseClaudeResponse(Response->GetContentAsString());
    if (!Decision.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[AIDirector] 파싱 실패 — 폴백 결정 사용"));
        Decision = FallbackDecision(LastContext);
    }

    ApplyAndLog(Decision);
}

// ─────────────────────────────────────────────────────────────────────
// 로컬 추론 서버 호출
// ─────────────────────────────────────────────────────────────────────

void AAIDirector::RequestDecisionFromLocalServer(const FSceneContext& Context)
{
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(LocalInferenceEndpoint);
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetContentAsString(BuildLocalServerRequestBody(Context));
    Request->OnProcessRequestComplete().BindUObject(this, &AAIDirector::OnLocalServerResponse);
    Request->ProcessRequest();
}

void AAIDirector::OnLocalServerResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
{
    if (!bSuccess || !Response.IsValid() || Response->GetResponseCode() != 200)
    {
        UE_LOG(LogTemp, Warning, TEXT("[AIDirector] 로컬 서버 실패 — 폴백 결정 사용"));
        ApplyAndLog(FallbackDecision(LastContext));
        return;
    }

    FDirectorDecision Decision = ParseLocalServerResponse(Response->GetContentAsString());
    if (!Decision.IsValid())
    {
        Decision = FallbackDecision(LastContext);
    }

    ApplyAndLog(Decision);
}

// ─────────────────────────────────────────────────────────────────────
// 결정 적용 및 로깅
// ─────────────────────────────────────────────────────────────────────

void AAIDirector::ApplyAndLog(const FDirectorDecision& Decision)
{
    // 샷 타입이 결정됐으면 카메라 위치를 계산해서 결정에 채워 넣습니다.
    FDirectorDecision FinalDecision = Decision;
    if (FinalDecision.ShotType != ECineShotType::None)
    {
        ComputeShotTransform(
            LastContext,
            FinalDecision.ShotType,
            FinalDecision.TargetCameraLocation,
            FinalDecision.TargetCameraRotation
        );
    }

    if (TargetCamera)
    {
        TargetCamera->ApplyDecision(FinalDecision);
    }

    CurrentShotDuration   = 0.0f;                       // 새 샷 시작 → 타이머 리셋
    LastAppliedShotType   = FinalDecision.ShotType;     // 다음 주기에 CurrentShotType으로 전달

    ExperienceLogger->LogExperience(LastContext, FinalDecision, 0.0f);
    OnDecisionMade.Broadcast(FinalDecision);

    UE_LOG(LogTemp, Log,
        TEXT("[AIDirector] 결정: %s (confidence=%.2f) — %s"),
        *ShotTypeToString(FinalDecision.ShotType),
        FinalDecision.Confidence,
        *FinalDecision.Reasoning);
}

// ─────────────────────────────────────────────────────────────────────
// Claude API JSON 직렬화 — 프롬프트 엔지니어링
// ─────────────────────────────────────────────────────────────────────

FString AAIDirector::BuildClaudeRequestBody(const FSceneContext& Context)
{
    // ── 씬 정보 텍스트 구성 ──────────────────────────────────────────
    FString SceneDesc = FString::Printf(
        TEXT("Scene type: %s\n")
        TEXT("Characters in scene: %d\n")
        TEXT("Current shot: %s (held for %.1f seconds)\n"),
        *SceneTypeToString(Context.SceneType),
        Context.Characters.Num(),
        *ShotTypeToString(Context.CurrentShotType),
        Context.CurrentShotDuration
    );

    // 캐릭터별 상태
    for (int32 i = 0; i < Context.Characters.Num(); ++i)
    {
        const FCineCharacterInfo& C = Context.Characters[i];
        float Speed = C.Velocity.Size();
        SceneDesc += FString::Printf(
            TEXT("  Character %d: %s, health=%.0f%%, speed=%.0f cm/s%s%s%s\n"),
            i,
            C.bIsPlayer ? TEXT("Player") : TEXT("NPC"),
            C.HealthNormalized * 100.0f,
            Speed,
            C.bIsSpeaking  ? TEXT(", speaking")  : TEXT(""),
            C.bIsInCombat  ? TEXT(", in combat")  : TEXT(""),
            C.bIsDead      ? TEXT(", DEAD")       : TEXT("")
        );
    }

    // ── 샷 선택지 목록 ────────────────────────────────────────────────
    const FString ShotList =
        TEXT("0:None, 1:ExtremeWide, 2:Wide, 3:Medium, 4:MediumCloseUp, ")
        TEXT("5:CloseUp, 6:ExtremeCloseUp, 7:OverShoulder, 8:TwoShot, ")
        TEXT("9:POV, 10:BirdsEye, 11:LowAngle, 12:Dutch");

    // ── 프롬프트 ─────────────────────────────────────────────────────
    FString UserContent = FString::Printf(
        TEXT("You are an expert film director AI. Choose the best cinematic camera shot ")
        TEXT("for the following game scene. Respond ONLY with a valid JSON object, no markdown.\n\n")
        TEXT("SCENE:\n%s\n")
        TEXT("AVAILABLE SHOT TYPES (id:name):\n%s\n\n")
        TEXT("Respond with this exact JSON format:\n")
        TEXT("{\"shot_type\":<id>, \"transition_type\":<0=Cut|1=Blend|2=Fade>, ")
        TEXT("\"transition_duration\":<seconds 0.3-2.0>, \"confidence\":<0.0-1.0>, ")
        TEXT("\"reasoning\":\"<brief reason>\"}"),
        *SceneDesc,
        *ShotList
    );

    // ── Claude API messages 포맷 ─────────────────────────────────────
    TSharedPtr<FJsonObject> Root = MakeShareable(new FJsonObject);
    Root->SetStringField(TEXT("model"),      TEXT("claude-opus-4-6"));
    Root->SetNumberField(TEXT("max_tokens"), 256);
    Root->SetStringField(TEXT("system"),
        TEXT("You are a professional film director AI assistant. "
             "Always respond with a single valid JSON object only. No explanation, no markdown."));

    TSharedPtr<FJsonObject> Message = MakeShareable(new FJsonObject);
    Message->SetStringField(TEXT("role"),    TEXT("user"));
    Message->SetStringField(TEXT("content"), UserContent);

    TArray<TSharedPtr<FJsonValue>> Messages;
    Messages.Add(MakeShareable(new FJsonValueObject(Message)));
    Root->SetArrayField(TEXT("messages"), Messages);

    FString Output;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
    FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
    return Output;
}

FString AAIDirector::BuildLocalServerRequestBody(const FSceneContext& Context)
{
    // 로컬 서버가 기대하는 state 벡터 (preprocessor.py STATE_COLUMNS 순서와 동일)
    TSharedPtr<FJsonObject> Root = MakeShareable(new FJsonObject);
    Root->SetNumberField(TEXT("scene_type"),      (int32)Context.SceneType);
    Root->SetNumberField(TEXT("character_count"), Context.Characters.Num());
    Root->SetNumberField(TEXT("current_shot"),    (int32)Context.CurrentShotType);
    Root->SetNumberField(TEXT("shot_duration"),   Context.CurrentShotDuration);
    Root->SetNumberField(TEXT("cam_x"),           Context.CameraLocation.X);
    Root->SetNumberField(TEXT("cam_y"),           Context.CameraLocation.Y);
    Root->SetNumberField(TEXT("cam_z"),           Context.CameraLocation.Z);

    FString Output;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
    FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
    return Output;
}

// ─────────────────────────────────────────────────────────────────────
// 응답 파싱
// ─────────────────────────────────────────────────────────────────────

FDirectorDecision AAIDirector::ParseClaudeResponse(const FString& ResponseBody)
{
    // Claude API 응답 구조:
    // { "content": [{ "type": "text", "text": "{...실제 JSON...}" }], ... }

    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[AIDirector] 응답 최상위 JSON 파싱 실패"));
        return FDirectorDecision();
    }

    // content 배열에서 첫 번째 text 항목 추출
    const TArray<TSharedPtr<FJsonValue>>* ContentArray = nullptr;
    if (!Root->TryGetArrayField(TEXT("content"), ContentArray) || ContentArray->IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[AIDirector] 'content' 필드를 찾을 수 없습니다"));
        return FDirectorDecision();
    }

    FString TextContent;
    for (const TSharedPtr<FJsonValue>& Item : *ContentArray)
    {
        TSharedPtr<FJsonObject> ItemObj = Item->AsObject();
        if (!ItemObj.IsValid()) continue;

        FString ItemType;
        if (ItemObj->TryGetStringField(TEXT("type"), ItemType) && ItemType == TEXT("text"))
        {
            ItemObj->TryGetStringField(TEXT("text"), TextContent);
            break;
        }
    }

    if (TextContent.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[AIDirector] text 콘텐츠가 비어 있습니다"));
        return FDirectorDecision();
    }

    // Claude가 반환한 JSON 문자열을 다시 파싱
    TSharedPtr<FJsonObject> DecisionJson;
    TSharedRef<TJsonReader<>> DecReader = TJsonReaderFactory<>::Create(TextContent);
    if (!FJsonSerializer::Deserialize(DecReader, DecisionJson) || !DecisionJson.IsValid())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[AIDirector] 결정 JSON 파싱 실패. 내용: %s"), *TextContent.Left(200));
        return FDirectorDecision();
    }

    return ParseDecisionFromJsonObject(DecisionJson);
}

FDirectorDecision AAIDirector::ParseLocalServerResponse(const FString& ResponseBody)
{
    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        return FDirectorDecision();
    }
    return ParseDecisionFromJsonObject(Root);
}

FDirectorDecision AAIDirector::ParseDecisionFromJsonObject(const TSharedPtr<FJsonObject>& JsonObject)
{
    FDirectorDecision Decision;

    int32 ShotTypeInt = 0;
    if (JsonObject->TryGetNumberField(TEXT("shot_type"), ShotTypeInt))
    {
        ShotTypeInt = FMath::Clamp(ShotTypeInt, 0, 12);
        Decision.ShotType = (ECineShotType)ShotTypeInt;
    }

    int32 TransitionTypeInt = 1; // 기본: Blend
    JsonObject->TryGetNumberField(TEXT("transition_type"), TransitionTypeInt);
    TransitionTypeInt = FMath::Clamp(TransitionTypeInt, 0, 2);
    Decision.TransitionType = (ECineTransitionType)TransitionTypeInt;

    double TransitionDuration = 0.8;
    JsonObject->TryGetNumberField(TEXT("transition_duration"), TransitionDuration);
    Decision.TransitionDuration = FMath::Clamp((float)TransitionDuration, 0.1f, 5.0f);

    double Confidence = 0.0;
    JsonObject->TryGetNumberField(TEXT("confidence"), Confidence);
    Decision.Confidence = FMath::Clamp((float)Confidence, 0.0f, 1.0f);

    JsonObject->TryGetStringField(TEXT("reasoning"), Decision.Reasoning);

    return Decision;
}

// ─────────────────────────────────────────────────────────────────────
// 규칙 기반 폴백 결정
// ─────────────────────────────────────────────────────────────────────

FDirectorDecision AAIDirector::FallbackDecision(const FSceneContext& Context)
{
    FDirectorDecision Decision;
    Decision.TransitionType     = ECineTransitionType::Blend;
    Decision.TransitionDuration = 0.8f;
    Decision.Confidence         = 0.4f;

    switch (Context.SceneType)
    {
        case ECineSceneType::Dialogue:
            Decision.ShotType  = ECineShotType::MediumCloseUp;
            Decision.Reasoning = TEXT("폴백: 대화 씬 → 미디엄 클로즈업");
            break;

        case ECineSceneType::Combat:
            Decision.ShotType  = ECineShotType::Wide;
            Decision.Reasoning = TEXT("폴백: 전투 씬 → 와이드");
            break;

        case ECineSceneType::Exploration:
            Decision.ShotType  = ECineShotType::Medium;
            Decision.Reasoning = TEXT("폴백: 탐험 씬 → 미디엄");
            break;

        case ECineSceneType::Death:
            Decision.ShotType       = ECineShotType::CloseUp;
            Decision.TransitionType = ECineTransitionType::Blend;
            Decision.TransitionDuration = 1.5f;
            Decision.Reasoning      = TEXT("폴백: 사망 씬 → 클로즈업");
            break;

        case ECineSceneType::Victory:
            Decision.ShotType  = ECineShotType::Wide;
            Decision.Reasoning = TEXT("폴백: 승리 씬 → 와이드");
            break;

        case ECineSceneType::Cutscene:
            Decision.ShotType  = ECineShotType::Medium;
            Decision.Reasoning = TEXT("폴백: 컷씬 → 미디엄");
            break;

        default:
            Decision.ShotType  = ECineShotType::Medium;
            Decision.Reasoning = TEXT("폴백: 기본 미디엄 샷");
            break;
    }

    return Decision;
}

// ─────────────────────────────────────────────────────────────────────
// 샷 타입 → 카메라 위치/방향 계산
// ─────────────────────────────────────────────────────────────────────

void AAIDirector::ComputeShotTransform(
    const FSceneContext& Context,
    ECineShotType ShotType,
    FVector& OutLocation,
    FRotator& OutRotation)
{
    // 포커스 캐릭터 결정: 플레이어 우선, 없으면 첫 번째 캐릭터
    const FCineCharacterInfo* FocusChar  = nullptr;
    const FCineCharacterInfo* SecondChar = nullptr;

    for (const FCineCharacterInfo& C : Context.Characters)
    {
        if (C.bIsPlayer) { FocusChar = &C; }
        else if (!SecondChar) { SecondChar = &C; }
    }
    if (!FocusChar && Context.Characters.Num() > 0)
        FocusChar = &Context.Characters[0];

    // 포커스 캐릭터가 없으면 현재 카메라 위치 유지
    if (!FocusChar)
    {
        OutLocation = Context.CameraLocation;
        OutRotation = Context.CameraRotation;
        return;
    }

    const FVector CharPos = FocusChar->Location;

    // 캐릭터가 바라보는 방향 벡터 (캐릭터 전방 기준)
    const FVector Forward = FocusChar->Rotation.Vector();
    const FVector Right   = FRotationMatrix(FocusChar->Rotation).GetUnitAxis(EAxis::Y);
    const FVector Up      = FVector::UpVector;

    // 카메라가 향할 목표점 (캐릭터 눈 높이)
    const float   EyeHeight = 170.0f;
    const FVector FacePos   = CharPos + Up * EyeHeight;

    switch (ShotType)
    {
        // ── 광각 계열 ──────────────────────────────────────────────
        case ECineShotType::ExtremeWide:
            OutLocation = CharPos - Forward * 1500.0f + Up * 800.0f;
            OutRotation = (FacePos - OutLocation).Rotation();
            break;

        case ECineShotType::Wide:
            OutLocation = CharPos - Forward * 700.0f + Up * 260.0f + Right * 60.0f;
            OutRotation = (FacePos - OutLocation).Rotation();
            break;

        // ── 미디엄 계열 ───────────────────────────────────────────
        case ECineShotType::Medium:
            OutLocation = CharPos - Forward * 280.0f + Up * 130.0f + Right * 40.0f;
            OutRotation = (FacePos - OutLocation).Rotation();
            break;

        case ECineShotType::MediumCloseUp:
            OutLocation = CharPos - Forward * 160.0f + Up * 140.0f + Right * 30.0f;
            OutRotation = (FacePos - OutLocation).Rotation();
            break;

        // ── 클로즈업 계열 ─────────────────────────────────────────
        case ECineShotType::CloseUp:
            OutLocation = CharPos - Forward * 90.0f + Up * 160.0f + Right * 20.0f;
            OutRotation = (FacePos - OutLocation).Rotation();
            break;

        case ECineShotType::ExtremeCloseUp:
            OutLocation = CharPos - Forward * 45.0f + Up * 170.0f + Right * 10.0f;
            OutRotation = (FacePos - OutLocation).Rotation();
            break;

        // ── 특수 앵글 계열 ────────────────────────────────────────
        case ECineShotType::BirdsEye:
            OutLocation = CharPos + Up * 1200.0f;
            OutRotation = FRotator(-90.0f, FocusChar->Rotation.Yaw, 0.0f);
            break;

        case ECineShotType::LowAngle:
            // 캐릭터 발 아래에서 올려다보는 각도
            OutLocation = CharPos - Forward * 200.0f - Up * 30.0f + Right * 30.0f;
            OutRotation = (FacePos - OutLocation).Rotation();
            break;

        case ECineShotType::Dutch:
            // 미디엄 위치에서 카메라를 20° 기울임
            OutLocation = CharPos - Forward * 250.0f + Up * 130.0f + Right * 40.0f;
            {
                FRotator Base = (FacePos - OutLocation).Rotation();
                OutRotation   = FRotator(Base.Pitch, Base.Yaw, 20.0f);
            }
            break;

        case ECineShotType::POV:
            // 캐릭터 눈 위치에서 바라보는 방향
            OutLocation = FacePos + Forward * 10.0f;
            OutRotation = FocusChar->Rotation;
            break;

        // ── 멀티 캐릭터 계열 ──────────────────────────────────────
        case ECineShotType::OverShoulder:
            if (SecondChar)
            {
                // 포커스 캐릭터의 어깨 뒤에서 SecondChar를 바라봄
                OutLocation = CharPos + Forward * 50.0f + Right * 65.0f + Up * 160.0f;
                OutRotation = (SecondChar->Location + Up * EyeHeight - OutLocation).Rotation();
            }
            else
            {
                OutLocation = CharPos - Forward * 100.0f + Right * 65.0f + Up * 160.0f;
                OutRotation = (FacePos - OutLocation).Rotation();
            }
            break;

        case ECineShotType::TwoShot:
            if (SecondChar)
            {
                // 두 캐릭터 중간 지점 옆에 카메라 배치
                FVector MidPoint    = (CharPos + SecondChar->Location) * 0.5f;
                FVector Dir         = (SecondChar->Location - CharPos).GetSafeNormal();
                FVector Perp        = FVector::CrossProduct(Dir, Up).GetSafeNormal();
                float   CharDist    = FVector::Dist(CharPos, SecondChar->Location);
                OutLocation = MidPoint + Perp * (CharDist * 0.9f) + Up * 180.0f;
                OutRotation = (MidPoint + Up * EyeHeight - OutLocation).Rotation();
            }
            else
            {
                // 두 번째 캐릭터 없으면 미디엄 샷으로 폴백
                OutLocation = CharPos - Forward * 280.0f + Up * 130.0f + Right * 40.0f;
                OutRotation = (FacePos - OutLocation).Rotation();
            }
            break;

        default:
            // None 이거나 알 수 없는 샷 → 카메라 위치 유지
            OutLocation = Context.CameraLocation;
            OutRotation = Context.CameraRotation;
            break;
    }
}

// ─────────────────────────────────────────────────────────────────────
// 유틸리티 — enum → 문자열
// ─────────────────────────────────────────────────────────────────────

FString AAIDirector::SceneTypeToString(ECineSceneType SceneType)
{
    switch (SceneType)
    {
        case ECineSceneType::Dialogue:    return TEXT("Dialogue");
        case ECineSceneType::Combat:      return TEXT("Combat");
        case ECineSceneType::Exploration: return TEXT("Exploration");
        case ECineSceneType::Cutscene:    return TEXT("Cutscene");
        case ECineSceneType::Death:       return TEXT("Death");
        case ECineSceneType::Victory:     return TEXT("Victory");
        default:                          return TEXT("Unknown");
    }
}

FString AAIDirector::ShotTypeToString(ECineShotType ShotType)
{
    switch (ShotType)
    {
        case ECineShotType::ExtremeWide:    return TEXT("ExtremeWide");
        case ECineShotType::Wide:           return TEXT("Wide");
        case ECineShotType::Medium:         return TEXT("Medium");
        case ECineShotType::MediumCloseUp:  return TEXT("MediumCloseUp");
        case ECineShotType::CloseUp:        return TEXT("CloseUp");
        case ECineShotType::ExtremeCloseUp: return TEXT("ExtremeCloseUp");
        case ECineShotType::OverShoulder:   return TEXT("OverShoulder");
        case ECineShotType::TwoShot:        return TEXT("TwoShot");
        case ECineShotType::POV:            return TEXT("POV");
        case ECineShotType::BirdsEye:       return TEXT("BirdsEye");
        case ECineShotType::LowAngle:       return TEXT("LowAngle");
        case ECineShotType::Dutch:          return TEXT("Dutch");
        default:                            return TEXT("None");
    }
}
