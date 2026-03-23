// Copyright Cine-AI-Director Team. All Rights Reserved.

#include "Director/SceneAnalyzer.h"
#include "Camera/ICineCharacterInterface.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

USceneAnalyzer::USceneAnalyzer()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void USceneAnalyzer::BeginPlay()
{
    Super::BeginPlay();
}

void USceneAnalyzer::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    TimeSinceLastAnalysis += DeltaTime;
}

FSceneContext USceneAnalyzer::CaptureContext()
{
    FSceneContext Context;
    Context.GameTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    Context.Characters = GatherCharacterInfos();
    Context.SceneType = ClassifyScene(Context.Characters);

    // 현재 카메라 위치는 AIDirector에서 채워줍니다.
    return Context;
}

TArray<FCineCharacterInfo> USceneAnalyzer::GatherCharacterInfos()
{
    TArray<FCineCharacterInfo> Infos;

    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), FoundActors);

    ACharacter* PlayerChar = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);

    for (AActor* Actor : FoundActors)
    {
        ACharacter* Character = Cast<ACharacter>(Actor);
        if (!Character) continue;

        FCineCharacterInfo Info;
        Info.Location = Character->GetActorLocation();
        Info.Rotation = Character->GetActorRotation();
        Info.bIsPlayer = (Character == PlayerChar);

        if (UMovementComponent* MoveComp = Character->GetMovementComponent())
        {
            Info.Velocity = MoveComp->Velocity;
        }

        // ───────────────────────────────────────────────────────────────
        // ICineCharacterInterface 연동
        // 캐릭터 블루프린트에서 이 인터페이스를 구현했으면 자동으로 사용합니다.
        // 구현 안 해도 기본값(풀피, 비대화, 비전투)으로 정상 작동합니다.
        // ───────────────────────────────────────────────────────────────
        if (Character->GetClass()->ImplementsInterface(UCineCharacterInterface::StaticClass()))
        {
            Info.HealthNormalized = ICineCharacterInterface::Execute_GetHealthNormalized(Character);
            Info.bIsSpeaking      = ICineCharacterInterface::Execute_IsSpeaking(Character);
            Info.bIsInCombat      = ICineCharacterInterface::Execute_IsInCombat(Character);
            Info.bIsDead          = ICineCharacterInterface::Execute_IsDead(Character);

            // 체력이 0이면 사망 처리
            if (Info.HealthNormalized <= 0.0f)
            {
                Info.bIsDead = true;
            }
        }
        else
        {
            // 인터페이스 미구현 시 기본값 유지 (HealthNormalized=1.0, bIsDead=false 등)
        }

        Infos.Add(Info);
    }

    return Infos;
}

ECineSceneType USceneAnalyzer::ClassifyScene(const TArray<FCineCharacterInfo>& Characters)
{
    // 수동 오버라이드가 설정되어 있으면 즉시 반환
    if (bOverrideSceneType)
    {
        return OverrideSceneType;
    }

    if (Characters.IsEmpty())
    {
        return ECineSceneType::Unknown;
    }

    // 각 상태 집계
    bool bAnyDead      = false;
    bool bAnySpeaking  = false;
    bool bAnyInCombat  = false;
    bool bAnyMoving    = false;
    int32 PlayerCount  = 0;

    for (const FCineCharacterInfo& Char : Characters)
    {
        if (Char.bIsDead || Char.HealthNormalized <= 0.0f) bAnyDead     = true;
        if (Char.bIsSpeaking)                              bAnySpeaking = true;
        if (Char.bIsPlayer)                                PlayerCount++;

        // ICineCharacterInterface 전투 플래그, 또는 속도 기반 판단
        if (Char.bIsInCombat)
        {
            bAnyInCombat = true;
        }
        else if (Char.Velocity.SizeSquared() > CombatVelocityThreshold * CombatVelocityThreshold)
        {
            bAnyMoving = true;
        }
    }

    // 우선순위: 사망 > 전투 > 대화 > 탐험 > 알 수 없음
    if (bAnyDead)                   return ECineSceneType::Death;
    if (bAnyInCombat)               return ECineSceneType::Combat;
    if (bAnySpeaking)               return ECineSceneType::Dialogue;
    if (bAnyMoving)                 return ECineSceneType::Exploration;
    // 캐릭터가 2명 이상이고 멈춰 있으면 대화 상황으로 간주
    if (Characters.Num() >= 2)      return ECineSceneType::Dialogue;

    return ECineSceneType::Exploration;
}

void USceneAnalyzer::SetSceneTypeOverride(ECineSceneType SceneType)
{
    bOverrideSceneType = true;
    OverrideSceneType  = SceneType;
}

void USceneAnalyzer::ClearSceneTypeOverride()
{
    bOverrideSceneType = false;
}
