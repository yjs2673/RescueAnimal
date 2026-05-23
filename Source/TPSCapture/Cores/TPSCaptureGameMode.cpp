#include "TPSCaptureGameMode.h"
#include "TPSCaptureCharacter.h"
#include "TPSPlayerController.h"
#include "UObject/ConstructorHelpers.h"

ATPSCaptureGameMode::ATPSCaptureGameMode()
{
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(
		TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter")
	);

	if (PlayerPawnBPClass.Class != nullptr)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}

	PlayerControllerClass = ATPSPlayerController::StaticClass();
}