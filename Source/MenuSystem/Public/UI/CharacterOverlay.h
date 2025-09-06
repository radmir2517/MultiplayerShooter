// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CharacterOverlay.generated.h"

class UImage;
class UTextBlock;
class UProgressBar;
/**
 * 
 */
UCLASS()
class MENUSYSTEM_API UCharacterOverlay : public UUserWidget
{
	GENERATED_BODY()

public:
	// прогресс бар здоровья
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UProgressBar> HealthBar;
	// текстовое значение здоровья
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> HealthText;
	
	// прогресс бар щита
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UProgressBar> ShieldBar;
	// текстовое значения щита
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> ShieldText;
	
	// текстовое значение очков
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> ScopeAmount;
	// текстовое значение Defeats
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> DefeatsAmount;
	// текстовое значение Defeats
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> WeaponAmmoAmount;
	// текстовое значение CarriedAmmo
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> CarriedAmmo;
	// текстовое значение MatchCountdownText
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> MatchCountdownText;
	// текстовое значение GrenadesAmount
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> GrenadesAmount;

	//  Картинка высокого пинга
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> HighPingImage;
	// анимация пинга
	UPROPERTY(meta=(BindWidgetAnim), Transient)
	TObjectPtr<UWidgetAnimation> HighPingAnimation;
};






































