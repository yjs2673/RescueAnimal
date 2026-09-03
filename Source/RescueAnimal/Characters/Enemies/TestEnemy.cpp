// Fill out your copyright notice in the Description page of Project Settings.


#include "TestEnemy.h"

// Sets default values
ATestEnemy::ATestEnemy()
{
	PrimaryActorTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void ATestEnemy::BeginPlay()
{
	Super::BeginPlay();
	
}

float ATestEnemy::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser
)
{
	UE_LOG(LogTemp, Warning, TEXT("Enemy Damaged: %f"), DamageAmount);
	return DamageAmount;
}
