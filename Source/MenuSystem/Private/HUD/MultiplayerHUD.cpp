// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/MultiplayerHUD.h"

#include "Blueprint/UserWidget.h"
#include "UI/AnnouncementWidget.h"

void AMultiplayerHUD::DrawHUD()
{
	Super::DrawHUD();

	if (GEngine)
	{	// получим размеры экрана и вычислим центр экрана
		FVector2D ViewportSize;
		GEngine->GameViewport->GetViewportSize(ViewportSize);
		FVector2D CenterScreen;
		CenterScreen.X = (ViewportSize.X * 0.5f);
		CenterScreen.Y = (ViewportSize.Y * 0.5f);

		float CurrentSpreadFactor = HUDPackage.SpreadFactor * HUDPackage.MaxSpreadMagnitude;
		// если есть текстура, то нарисуем ее на экране 
	    if (HUDPackage.CrosshairsCenter)
	    {
	   		DrawAimTexture(HUDPackage.CrosshairsCenter, CenterScreen, 0.f, 0.f, HUDPackage.CrosshairsColor);
	    }
		if (HUDPackage.CrosshairsTop)
		{
			DrawAimTexture(HUDPackage.CrosshairsTop, CenterScreen, 0.f, CurrentSpreadFactor,HUDPackage.CrosshairsColor);
		}
		if (HUDPackage.CrosshairsBottom)
		{
			DrawAimTexture(HUDPackage.CrosshairsBottom, CenterScreen, 0.f, -CurrentSpreadFactor,HUDPackage.CrosshairsColor);
		}
		if (HUDPackage.CrosshairsLeft)
		{
			DrawAimTexture(HUDPackage.CrosshairsLeft, CenterScreen, CurrentSpreadFactor, 0.f,HUDPackage.CrosshairsColor);
		}
		if (HUDPackage.CrosshairsRight)
		{
			DrawAimTexture(HUDPackage.CrosshairsRight, CenterScreen, -CurrentSpreadFactor, 0.f,HUDPackage.CrosshairsColor);
		}
	}
}

UUserWidget* AMultiplayerHUD::CreateOverlayWidget()
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (IsValid(PlayerController))
	{
		CharacterOverlayWidget = CreateWidget(PlayerController,CharacterOverlayClass);
		CharacterOverlayWidget->AddToViewport();
		return CharacterOverlayWidget;
	}
	return nullptr;
}

void AMultiplayerHUD::BeginPlay()
{
	Super::BeginPlay();
	// получем контроллер и создадим виджет/ теперь это в MultiplayerPlayerController::OnMatchStateSet
	//CreateOverlayWidget();
}

void AMultiplayerHUD::SetHUDPackage(FHUDPackage InHUDPackage)
{
	HUDPackage = InHUDPackage;
}


void AMultiplayerHUD::DrawAimTexture(UTexture2D* Texture, FVector2D& CenterScreen, float FactorSpreadOfAim_X, float FactorSpreadOfAim_Y, FLinearColor CrosshairColor)
{	// получим размеры текстуры
	int32 TextureSizeX = Texture->GetSizeX();
	int32 TextureSizeY = Texture->GetSizeY();
	// получим крайнее верхнюю точку для спавна картинки
	float ScreenX = CenterScreen.X - TextureSizeX / 2 - FactorSpreadOfAim_X;
	float ScreenY = CenterScreen.Y - TextureSizeY / 2 - FactorSpreadOfAim_Y;
	// применим функцию которая есть в UHUD для отрисовки прицела
	DrawTexture(Texture,
		ScreenX,
		ScreenY,
		TextureSizeX,
		TextureSizeY,
		0.f,
		0.f,
		1.f,
		1.f,
		CrosshairColor);
}

UAnnouncementWidget* AMultiplayerHUD::CreateAnnouncementWidget()
{	// 1.4 получим контроллер
	APlayerController* PlayerController = GetOwningPlayerController();
	if (AnnouncementWidgetClass && PlayerController && !AnnouncementWidget)
	{	// 1.5 создадим виджет и выведем его на экран
		AnnouncementWidget = CreateWidget<UAnnouncementWidget>(PlayerController,AnnouncementWidgetClass);
		if (AnnouncementWidget)
		{
			AnnouncementWidget->AddToViewport();
			return AnnouncementWidget;
		}
	}
	return nullptr;
}
