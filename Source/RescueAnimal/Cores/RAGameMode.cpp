#include "RAGameMode.h"
#include "RACharacter.h"
#include "RAPlayerController.h"
#include "UObject/ConstructorHelpers.h"

ARAGameMode::ARAGameMode()
{
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(
		TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter")
	);

	if (PlayerPawnBPClass.Class != nullptr)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}

	PlayerControllerClass = ARAPlayerController::StaticClass();
}