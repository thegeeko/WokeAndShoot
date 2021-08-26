// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameFramework/GameSession.h"
#include "UObject/ConstructorHelpers.h"
#include "WokeAndShoot/ServerComponents/MyPlayerState.h"
#include "WokeAndShootHUD.h"
#include "WokeAndShootCharacter.h"
#include "WokeAndShootPlayerController.h"
#include "WokeAndShootGameMode.h"

AWokeAndShootGameMode::AWokeAndShootGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = AWokeAndShootHUD::StaticClass();
}

void AWokeAndShootGameMode::PawnKilled(AController* Killed, AController* Killer) 
{
	AWokeAndShootPlayerController* KillerController = Cast<AWokeAndShootPlayerController>(Killer);
	if(KillerController == nullptr){return;}
	AWokeAndShootPlayerController* KilledController = Cast<AWokeAndShootPlayerController>(Killed);
	if(KilledController == nullptr){return;}
	//Despawning body
	FTimerDelegate TimerDel;
	TimerDel.BindUFunction(this,FName("DespawnBody"), KilledController);
	GetWorld()->GetTimerManager().SetTimer(DespawnBodyTH, TimerDel, 3.f, false);
	UpdateKillerName(KilledController, KillerController);
	UpdateScore(Killer->GetUniqueID());
}

void AWokeAndShootGameMode::BeginPlay() 
{
	
}

void AWokeAndShootGameMode::UpdateKillerName(AWokeAndShootPlayerController* KilledController, AWokeAndShootPlayerController* KillerController) 
{
	KilledController->GetPlayerState<AMyPlayerState>()->LastKilledBy = KillerController->PlayerName;

	//Only for when playing on the server as a client
	if(KilledController->HasAuthority())
	{
		KilledController->DisplayDeadWidget(KillerController->PlayerName);
	}
}

void AWokeAndShootGameMode::UpdateScore(uint32 KillerID) 
{
	
	int32 CurrentPlayerScore = Players.Find(KillerID)->Score;
	CurrentPlayerScore++;
	Players.Find(KillerID)->Score = CurrentPlayerScore;
	if(ScoreLimit && CurrentPlayerScore == MaxScore)
	{
		GameOver = true;
		RestartGame();
	}
}

void AWokeAndShootGameMode::RestartGame() 
{
	for( FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator )
	{
		APlayerController* PlayerController = Iterator->Get();
		if(PlayerController == nullptr)
		{
			return;
		}
		APawn* PlayerPawn = PlayerController->GetPawn();
		if(PlayerPawn != nullptr)
		{
			PlayerPawn->DetachFromControllerPendingDestroy();
			PlayerPawn->Destroy();
		}
	}
}

void AWokeAndShootGameMode::DespawnBody(AWokeAndShootPlayerController* Killed) 
{
    PlayersAlive--;
	if(APawn* KilledPawn = Killed->GetPawn())
	{
		KilledPawn->DetachFromControllerPendingDestroy();
		KilledPawn->Destroy();
	}
	Respawn(Killed);
}
 
void AWokeAndShootGameMode::Respawn(AWokeAndShootPlayerController* PlayerController) 
{
	if(GameOver){return;}
    //Replace with optimal spawn algo
    int32 SpawnIndex = FMath::RandRange(0,SpawnLocations.Num()-1);
    FVector SpawnLocation = SpawnLocations[SpawnIndex];
    FRotator SpawnRotation = FRotator(0,0,0);
    FActorSpawnParameters SpawnParams;
    AWokeAndShootCharacter* PlayerCharacter = GetWorld()->SpawnActor<AWokeAndShootCharacter>(DefaultPawnClass,SpawnLocation,SpawnRotation);
    if(PlayerCharacter != nullptr)
    {
        PlayerController->GetPlayerState<AMyPlayerState>()->NewPawn = PlayerCharacter;
		
		//Only for when playing on the server as a client
		if(PlayerController->HasAuthority())
		{
			PlayerController->Possess(PlayerCharacter);
			PlayerController->ClearDeadWidget();
		}
    }
}
