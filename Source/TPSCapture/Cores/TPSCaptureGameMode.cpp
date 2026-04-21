// Copyright Epic Games, Inc. All Rights Reserved.

#include "TPSCaptureGameMode.h"
#include "TPSCaptureCharacter.h"
#include "UObject/ConstructorHelpers.h"

ATPSCaptureGameMode::ATPSCaptureGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
