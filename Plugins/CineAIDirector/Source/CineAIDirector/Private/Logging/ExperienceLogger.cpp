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
    // 남은 항목 플러시 후, 지연 리워드 업데이트 사항 반영
    FlushToDisk();
    FlushRewardUpdates();
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

    CurrentLogFilePath    = FPaths::Combine(BaseDir, FileName);
    RewardUpdateFilePath  = FPaths::Combine(BaseDir,
        FString::Printf(TEXT("reward_updates_%s.jsonl"), *CurrentSessionID));

    UE_LOG(LogTemp, Log, TEXT("[ExperienceLogger] 로그 파일: %s"), *CurrentLogFilePath);
}

FString UExperienceLogger::LogExperience(const FSceneContext& Context, const FDirectorDecision& Decision, float InitialReward)
{
    FString ID = GenerateID();

    TSharedPtr<FJsonObject> Entry = MakeShareable(new FJsonObject);
    Entry->SetStringField(TEXT("id"),        ID);
    Entry->SetNumberField(TEXT("timestamp"), Context.GameTime);
    Entry->SetNumberField(TEXT("reward"),    InitialReward);

    // State
    TSharedPtr<FJsonObject> State = MakeShareable(new FJsonObject);
    State->SetNumberField(TEXT("scene_type"),      (int32)Context.SceneType);
    State->SetNumberField(TEXT("character_count"), Context.Characters.Num());
    State->SetNumberField(TEXT("current_shot"),    (int32)Context.CurrentShotType);
    State->SetNumberField(TEXT("shot_duration"),   Context.CurrentShotDuration);
    State->SetNumberField(TEXT("cam_x"),           Context.CameraLocation.X);
    State->SetNumberField(TEXT("cam_y"),           Context.CameraLocation.Y);
    State->SetNumberField(TEXT("cam_z"),           Context.CameraLocation.Z);
    Entry->SetObjectField(TEXT("state"), State);

    // Action
    TSharedPtr<FJsonObject> Action = MakeShareable(new FJsonObject);
    Action->SetNumberField(TEXT("shot_type"),          (int32)Decision.ShotType);
    Action->SetNumberField(TEXT("transition_type"),    (int32)Decision.TransitionType);
    Action->SetNumberField(TEXT("transition_duration"), Decision.TransitionDuration);
    Action->SetNumberField(TEXT("confidence"),          Decision.Confidence);
    Action->SetStringField(TEXT("reasoning"),           Decision.Reasoning);
    Entry->SetObjectField(TEXT("action"), Action);

    PendingEntries.Add(Entry);

    if (PendingEntries.Num() >= 10)
    {
        FlushToDisk();
    }

    return ID;
}

void UExperienceLogger::UpdateReward(const FString& ExperienceID, float Reward)
{
    if (ExperienceID.IsEmpty()) return;

    // 리워드 업데이트를 인메모리 맵에 누적합니다.
    // 세션 종료 시(EndPlay) 별도 JSONL 파일에 일괄 저장됩니다.
    // Python 파이프라인(collector.py)이 두 파일을 병합해서 최종 reward를 사용합니다.
    PendingRewardUpdates.Add(ExperienceID, Reward);

    UE_LOG(LogTemp, Verbose,
        TEXT("[ExperienceLogger] 리워드 업데이트 예약 — ID: %s, Reward: %.3f"),
        *ExperienceID, Reward);
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

    FFileHelper::SaveStringToFile(
        Output,
        *CurrentLogFilePath,
        FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
        &IFileManager::Get(),
        FILEWRITE_Append
    );

    PendingEntries.Empty();
}

void UExperienceLogger::FlushRewardUpdates()
{
    if (PendingRewardUpdates.IsEmpty()) return;

    // reward_updates_<session>.jsonl 에 {id, reward} 쌍을 저장합니다.
    // Python 파이프라인이 이 파일을 읽어 experience_log의 reward를 덮어씁니다.
    FString Output;
    for (const auto& Pair : PendingRewardUpdates)
    {
        TSharedPtr<FJsonObject> Entry = MakeShareable(new FJsonObject);
        Entry->SetStringField(TEXT("id"),     Pair.Key);
        Entry->SetNumberField(TEXT("reward"), Pair.Value);

        FString Line;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Line);
        FJsonSerializer::Serialize(Entry.ToSharedRef(), Writer);
        Output += Line + TEXT("\n");
    }

    FFileHelper::SaveStringToFile(
        Output,
        *RewardUpdateFilePath,
        FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
        &IFileManager::Get(),
        FILEWRITE_Append
    );

    UE_LOG(LogTemp, Log,
        TEXT("[ExperienceLogger] 리워드 업데이트 %d건 저장: %s"),
        PendingRewardUpdates.Num(),
        *RewardUpdateFilePath);

    PendingRewardUpdates.Empty();
}

FString UExperienceLogger::GenerateID() const
{
    return FGuid::NewGuid().ToString(EGuidFormats::Digits);
}
