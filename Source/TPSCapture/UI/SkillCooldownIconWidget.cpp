#include "SkillCooldownIconWidget.h"

#include "Components/Image.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "Widgets/InvalidateWidgetReason.h"

#include "PlayerSkillComponent.h"
#include "TPSCaptureCharacter.h"

#pragma region Lifecycle

void USkillCooldownIconWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ApplyIconTexture();
	InitializeCooldownMaterial();

	if (bAutoBindOwningPlayerSkillComponent)
	{
		ResolveSkillComponent();
	}
	else
	{
		BindCooldownEvents();
	}

	RefreshCooldown();
}

void USkillCooldownIconWidget::NativeDestruct()
{
	StopCooldownRefreshTimer();
	StopSkillComponentBindRetryTimer();
	UnbindCooldownEvents();

	Super::NativeDestruct();
}

#pragma endregion Lifecycle

#pragma region Setup

void USkillCooldownIconWidget::SetupSkillCooldownIcon(
	EWeaponType InSkillWeaponType,
	UTexture2D* InSkillIconTexture,
	UPlayerSkillComponent* InSkillComponent)
{
	SkillWeaponType = InSkillWeaponType;
	SkillIconTexture = InSkillIconTexture;

	if (InSkillComponent)
	{
		BindSkillComponent(InSkillComponent);
	}
	else if (bAutoBindOwningPlayerSkillComponent)
	{
		ResolveSkillComponent();
	}

	ApplyIconTexture();
	InitializeCooldownMaterial();
	RefreshCooldown();
}

#pragma endregion Setup

#pragma region Cooldown Data

void USkillCooldownIconWidget::SetSkillWeaponType(EWeaponType InSkillWeaponType)
{
	SkillWeaponType = InSkillWeaponType;
	RefreshCooldown();
}

void USkillCooldownIconWidget::SetSkillIconTexture(UTexture2D* InSkillIconTexture)
{
	SkillIconTexture = InSkillIconTexture;
	ApplyIconTexture();
}

void USkillCooldownIconWidget::BindSkillComponent(UPlayerSkillComponent* InSkillComponent)
{
	if (BoundSkillComponent == InSkillComponent)
	{
		RefreshCooldown();
		return;
	}

	UnbindCooldownEvents();
	BoundSkillComponent = InSkillComponent;
	BindCooldownEvents();
	RefreshCooldown();
}

void USkillCooldownIconWidget::RefreshCooldown()
{
	if (!BoundSkillComponent && bAutoBindOwningPlayerSkillComponent)
	{
		ResolveSkillComponent();
	}

	const float CooldownPercent = BoundSkillComponent
		? BoundSkillComponent->GetCooldownPercent(SkillWeaponType)
		: 0.0f;

	ApplyCooldownPercent(CooldownPercent);
}

#pragma endregion Cooldown Data

#pragma region Cooldown Material

void USkillCooldownIconWidget::InitializeCooldownMaterial()
{
	if (!CooldownOverlayImage)
	{
		return;
	}

	if (CooldownOverlayMaterial)
	{
		CooldownOverlayImage->SetBrushFromMaterial(CooldownOverlayMaterial);
	}

	CooldownOverlayMID = CooldownOverlayImage->GetDynamicMaterial();
}

#pragma endregion Cooldown Material

#pragma region Skill Component Binding

void USkillCooldownIconWidget::ResolveSkillComponent()
{
	if (ATPSCaptureCharacter* PlayerCharacter = GetPlayerCharacter())
	{
		if (UPlayerSkillComponent* PlayerSkillComponent = PlayerCharacter->GetPlayerSkillComponent())
		{
			BindSkillComponent(PlayerSkillComponent);
			StopSkillComponentBindRetryTimer();
			return;
		}
	}

	if (bAutoBindOwningPlayerSkillComponent)
	{
		StartSkillComponentBindRetryTimer();
	}
}

void USkillCooldownIconWidget::BindCooldownEvents()
{
	if (!BoundSkillComponent)
	{
		return;
	}

	BoundSkillComponent->OnSkillCooldownStarted.AddUniqueDynamic(
		this,
		&USkillCooldownIconWidget::HandleSkillCooldownStarted
	);
}

void USkillCooldownIconWidget::UnbindCooldownEvents()
{
	if (!BoundSkillComponent)
	{
		return;
	}

	BoundSkillComponent->OnSkillCooldownStarted.RemoveDynamic(
		this,
		&USkillCooldownIconWidget::HandleSkillCooldownStarted
	);
}

#pragma endregion Skill Component Binding

#pragma region Appearance

void USkillCooldownIconWidget::ApplyCooldownPercent(float CooldownPercent)
{
	const float PreviousCooldownPercent = DisplayedCooldownPercent;
	DisplayedCooldownPercent = FMath::Clamp(CooldownPercent, 0.0f, 1.0f);
	const bool bIsCooldownActive = DisplayedCooldownPercent > KINDA_SMALL_NUMBER;
	const bool bCooldownVisualChanged = !FMath::IsNearlyEqual(PreviousCooldownPercent, DisplayedCooldownPercent, 0.001f);

	if (!CooldownOverlayMID)
	{
		InitializeCooldownMaterial();
	}

	if (CooldownOverlayMID)
	{
		CooldownOverlayMID->SetScalarParameterValue(CooldownPercentParameterName, DisplayedCooldownPercent);
	}

	if (!CooldownOverlayImage)
	{
		return;
	}

	if (bHideOverlayWhenReady)
	{
		CooldownOverlayImage->SetVisibility(bIsCooldownActive ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
	else
	{
		CooldownOverlayImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	CooldownOverlayImage->SetRenderOpacity(bIsCooldownActive ? CooldownOverlayOpacity : ReadyOverlayOpacity);

	if (bIsCooldownActive)
	{
		StartCooldownRefreshTimer();
	}
	else
	{
		StopCooldownRefreshTimer();
	}

	if (bCooldownVisualChanged)
	{
		Invalidate(EInvalidateWidgetReason::Paint);
	}
}

void USkillCooldownIconWidget::ApplyIconTexture()
{
	if (!SkillIconImage)
	{
		return;
	}

	if (SkillIconTexture)
	{
		SkillIconImage->SetBrushFromTexture(SkillIconTexture, true);
		SkillIconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		SkillIconImage->SetVisibility(ESlateVisibility::Hidden);
	}
}

#pragma endregion Appearance

#pragma region Cooldown Refresh Timer

void USkillCooldownIconWidget::HandleSkillCooldownStarted(EWeaponType CooldownSkillWeaponType)
{
	if (CooldownSkillWeaponType != SkillWeaponType)
	{
		return;
	}

	RefreshCooldown();
	StartCooldownRefreshTimer();
}

void USkillCooldownIconWidget::StartCooldownRefreshTimer()
{
	UWorld* World = GetWorld();
	if (!World || World->GetTimerManager().IsTimerActive(CooldownRefreshTimerHandle))
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		CooldownRefreshTimerHandle,
		this,
		&USkillCooldownIconWidget::RefreshCooldown,
		FMath::Max(0.01f, CooldownRefreshInterval),
		true
	);
}

void USkillCooldownIconWidget::StopCooldownRefreshTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CooldownRefreshTimerHandle);
	}
}

#pragma endregion Cooldown Refresh Timer

#pragma region Skill Component Binding Retry

void USkillCooldownIconWidget::StartSkillComponentBindRetryTimer()
{
	UWorld* World = GetWorld();
	if (!World || World->GetTimerManager().IsTimerActive(SkillComponentBindRetryTimerHandle))
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		SkillComponentBindRetryTimerHandle,
		this,
		&USkillCooldownIconWidget::ResolveSkillComponent,
		FMath::Max(0.05f, SkillComponentBindRetryInterval),
		true
	);
}

void USkillCooldownIconWidget::StopSkillComponentBindRetryTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SkillComponentBindRetryTimerHandle);
	}
}

#pragma endregion Skill Component Binding Retry

#pragma region Player Access

ATPSCaptureCharacter* USkillCooldownIconWidget::GetPlayerCharacter() const
{
	APawn* OwningPawn = GetOwningPlayerPawn();
	if (ATPSCaptureCharacter* PlayerCharacter = Cast<ATPSCaptureCharacter>(OwningPawn))
	{
		return PlayerCharacter;
	}

	return Cast<ATPSCaptureCharacter>(
		UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)
	);
}

#pragma endregion Player Access
