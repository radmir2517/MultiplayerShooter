// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/OverheadWidget.h"

#include "Components/TextBlock.h"

void UOverheadWidget::ShowLocalRole(APawn* InPawn)
{
	if (!IsValid(InPawn)) return;
	// получим роль
	ENetRole LocalRole = InPawn->GetRemoteRole();
	FString LocalRoleString;
	// и в зависимости какая роль присвоим текст
	switch (LocalRole)
	{
	case  ENetRole::ROLE_Authority:
		LocalRoleString = "Authority";
		break;
	case  ENetRole::ROLE_AutonomousProxy:
		LocalRoleString = "AutonomousProxy";
		break;
	case  ENetRole::ROLE_SimulatedProxy:
		LocalRoleString = "SimulatedProxy";
		break;
	case ENetRole::ROLE_None:
		LocalRoleString = "None";
		break;
	}
	LocalRoleString = FString::Printf(TEXT("LocalRole: %s"), *LocalRoleString);
	// и в зависимости какая роль присвоим текст
	NetworkRoleBlock->SetText(FText::FromString(LocalRoleString));
}
