// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OverheadWidget.generated.h"

class UTextBlock;
/**
 * 
 */
UCLASS()
class MENUSYSTEM_API UOverheadWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// функция получения роли и задания текстовому блоку текста
	UFUNCTION(BlueprintCallable)
	void ShowLocalRole(APawn* InPawn);
private:
	// текстовый блок
	UPROPERTY(VisibleAnywhere,meta = (BindWidget))
	TObjectPtr<UTextBlock> NetworkRoleBlock;
};
