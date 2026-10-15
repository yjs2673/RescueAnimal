#pragma once

#include "CoreMinimal.h"
#include "RACharacter.h"
#include "EnemyPerfTarget.generated.h"

// Test-map-only target. Uses the production target type, but deliberately skips player damage processing.
UCLASS()
class RESCUEANIMAL_API AEnemyPerfTarget : public ARACharacter
{
	GENERATED_BODY()

public:
	AEnemyPerfTarget();

	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

protected:
	virtual void BeginPlay() override;
};
