// Copyright Cine-AI-Director Team. All Rights Reserved.

#include "Logging/ExperienceLogger.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonWriter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Guid.h"

UExperienceLogger::UExperienceLogger()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UExperienceLogger::BeginPlay()
{
    Super::BeginPlay();
    CurrentSessionID = FGuid::NewGuid().ToString();
    InitLogFile();
}

void UExperienceLogger::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    FlushToDisk();
    Super::EndPlay(EndPlayReason);
}

void UExperienceLogger::InitLogFile()
{
    FString BaseDir = LogDirectory.IsEmpty()
        ? FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("CineAILogs"))
        : LogDirectory;

    IFileManager::Get().MakeDirectory(*BaseDir, true);

    FString FileName = bSeparateBySession
        ? FString::Printf(TEXT("session_%s.jsonl"), *CurrentSessionID)
        : TEXT("experience_log.jsonl");

    CurrentLogFilePath = FPaths::Combine(BaseDir, FileName);
    UE_LOG(LogTemp, Log, TEXT("[ExperienceLogger] 로그 파일: %s"), *CurrentLogFilePath);
}

FString UExperienceLogger::LogExperience(const FSceneContext& Context, const FDirectorDecision& Decision, float InitialReward)
{
    FString ID = GenerateID();

    // State + Action + Reward 를 JSON으로 직렬화
    TSharedPtr<FJsonObject> Entry = MakeShareable(new FJsonObject);
    Entry->SetStringField(TEXT("id"), ID);
    Entry->SetNumberField(TEXT("timestamp"), Context.GameTime);
    Entry->SetNumberField(TEXT("reward"), InitialReward);

    // State
    TSharedPtr<FJsonObject> State = MakeShareable(new FJsonObject);
    State->SetNumberField(TEXT("scene_type"), (int32)Context.SceneType);
    State->SetNumberField(TEXT("character_count"), Context.Characters.Num());
    State->SetNumberField(TEXT("current_shot"), (int32)Context.CurrentShotType);
    State->SetNumberField(TEXT("shot_duration"), Context.CurrentShotDuration);
    State->SetNumberField(TEXT("cam_x"), Context.CameraLocation.X);
    State->SetNumberField(TEXT("cam_y"), Context.CameraLocation.Y);
    State->SetNumberField(TEXT("cam_z"), Context.CameraLocation.Z);
    Entry->SetObjectField(TEXT("state"), State);

    // Action
    TSharedPtr<FJsonObject> Action = MakeShareable(new FJsonObject);
    Action->SetNumberField(TEXT("shot_type"), (int32)Decision.ShotType);
    Action->SetNumberField(TEXT("transition_type"), (int32)Decision.TransitionType);
    Action->SetNumberField(TEXT("transition_duration"), Decision.TransitionDuration);
    Action->SetNumberField(TEXT("confidence"), Decision.Confidence);
    Action->SetStringField(TEXT("reasoning"), Decision.Reasoning);
    Entry->SetObjectField(TEXT("action"), Action);

    PendingEntries.Add(Entry);

    // 10개마다 자동 플러시
    if (PendingEntries.Num() >= 10)
    {
        FlushToDisk();
    }

    return ID;
}

void UExperienceLogger::UpdateReward(const FString& ExperienceID, float Reward)
{
    // TODO: [ML 담당] 파일에서 해당 ID를 찾아 리워드를 업데이트하는 로직 구현
    // 현재는 로그만 남깁니다.
    UE_LOG(LogTemp, Log, TEXT("[ExperienceLogger] 리워드 업데이트 - ID: %s, Reward: %.3f"), *ExperienceID, Reward);
}

void UExperienceLogger::FlushToDisk()
{
    if (PendingEntries.IsEmpty()) return;

    FString Output;
    for (const TSharedPtr<FJsonObject>& Entry : PendingEntries)
    {
        FString Line;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Line);
        FJsonSerializer::Serialize(Entry.ToSharedRef(), Writer);
        Output += Line + TEXT("\n");
    }

    // JSONL (JSON Lines) 포맷으로 append 저장
    FFileHelper::SaveStringToFile(Output, *CurrentLogFilePath,
        FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
        &IFileManager::Get(), FILEWRITE_Append);

    PendingEntries.Empty();
}

FString UExperienceLogger::GenerateID() const
{
    return FGuid::NewGuid().ToString(EGuidFormats::Digits);
}
