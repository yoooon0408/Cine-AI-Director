// Copyright Cine-AI-Director Team. All Rights Reserved.

#include "Director/SceneAnalyzer.h"
#include "GameFramework/Character.h"
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

    // 레벨의 모든 Character 액터를 수집
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), FoundActors);

    for (AActor* Actor : FoundActors)
    {
        ACharacter* Character = Cast<ACharacter>(Actor);
        if (!Character) continue;

        FCineCharacterInfo Info;
        Info.Location = Character->GetActorLocation();
        Info.Rotation = Character->GetActorRotation();

        if (UMovementComponent* MoveComp = Character->GetMovementComponent())
        {
            Info.Velocity = MoveComp->Velocity;
        }

        // 플레이어 캐릭터인지 체크
        Info.bIsPlayer = (Character == UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));

        // TODO: 체력/대화 상태는 게임 캐릭터 클래스에서 가져오도록 수정 필요
        // [언리얼 담당] 여기에 게임 캐릭터의 체력/대화 인터페이스를 연결해주세요.

        Infos.Add(Info);
    }

    return Infos;
}

ECineSceneType USceneAnalyzer::ClassifyScene(const TArray<FCineCharacterInfo>& Characters)
{
    // TODO: [언리얼 담당] 게임 상황에 맞게 분류 로직을 구체화해주세요.
    // 예시: 적이 가까이 있으면 Combat, 캐릭터가 서 있으면 Dialogue 등

    bool bAnyMovingFast = false;
    for (const FCineCharacterInfo& Char : Characters)
    {
        if (Char.Velocity.SizeSquared() > 200.0f * 200.0f)
        {
            bAnyMovingFast = true;
            break;
        }
    }

    if (bAnyMovingFast)
    {
        return ECineSceneType::Exploration;
    }

    return ECineSceneType::Dialogue;
}
