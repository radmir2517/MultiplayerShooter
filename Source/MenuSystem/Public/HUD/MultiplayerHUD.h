// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MultiplayerHUD.generated.h"

class UAnnouncementWidget;
// структура с картинками частей прицела
USTRUCT()
struct FHUDPackage
{
	GENERATED_BODY()
	UPROPERTY()
	TObjectPtr<UTexture2D> CrosshairsCenter;
	UPROPERTY()
	TObjectPtr<UTexture2D> CrosshairsRight;
	UPROPERTY()
	TObjectPtr<UTexture2D> CrosshairsLeft;
	UPROPERTY()
	TObjectPtr<UTexture2D> CrosshairsTop;
	UPROPERTY()
	TObjectPtr<UTexture2D> CrosshairsBottom;
	UPROPERTY()
	float MaxSpreadMagnitude = 0;
	UPROPERTY()
	float SpreadFactor = 0.f;
	UPROPERTY()
	FLinearColor CrosshairsColor;
};
UCLASS()
class MENUSYSTEM_API AMultiplayerHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	// функция которая будет задавать структуру полученную с оружия с готовыми картинками прицела
	void SetHUDPackage(FHUDPackage InHUDPackage);
	// функция получения виджета на Оверлей
	FORCEINLINE UUserWidget* GetCharacterOverlayWidget() {return CharacterOverlayWidget;}
	FORCEINLINE UAnnouncementWidget* GetAnnouncementWidget() {return AnnouncementWidget;}
	// получим контроллер и создадим виджет
	UUserWidget* CreateOrGetOverlayWidget();
	// 1.3 получим контроллер и создадим виджет
	UFUNCTION()
	UAnnouncementWidget* CreateAnnouncementWidget();

protected:
	virtual void BeginPlay() override;

private:
	// функция где получаем размеры текстуры и применяем функцию DrawTexture
	void DrawAimTexture(UTexture2D* Texture, FVector2D& CenterScreen, float FactorSpreadOfAim_X, float FactorSpreadOfAim_Y, FLinearColor CrosshairColor );
	
	FHUDPackage HUDPackage;
	
	// класс виджета оверлея персонажа
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UUserWidget> CharacterOverlayClass;
	// указатель в котором будем содержаться записанный виджет
	TObjectPtr<UUserWidget> CharacterOverlayWidget;

	/*
	 * Анонсирующий виджет
	 */
	// 1.1 класс виджета Анонсирующего виджета
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UUserWidget> AnnouncementWidgetClass;
	// 1.2 указатель в котором будем содержаться записанный виджет
	TObjectPtr<UAnnouncementWidget> AnnouncementWidget;

};






































