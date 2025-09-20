// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerComponent/LagCompensationComponent.h"

#include "Character/MultiplayerCharacter.h"
#include "Components/BoxComponent.h"


ULagCompensationComponent::ULagCompensationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}

void ULagCompensationComponent::ShowFramePackage(const FFramePackage& FramePackage, const FColor& Color)
{
	UWorld* World = GetWorld();
	//27.5 профдемся по карте с коробки и визуализируем их
	for (auto HitBox : FramePackage.HitBoxInfo)
	{
		DrawDebugBox(World,
			HitBox.Value.Location,
			HitBox.Value.BoxEntent,
			HitBox.Value.Rotation.Quaternion(),
			Color,
			true);
	}
}

void ULagCompensationComponent::SafeFramePackage(FFramePackage& FramePackage)
{
	if (!MultiplayerCharacter) MultiplayerCharacter = Cast<AMultiplayerCharacter>(GetOwner());
	if (!MultiplayerCharacter) return;
	// 27.2 получим время
	FramePackage.Time = GetWorld()->GetTimeSeconds();
	// 27.3 // пройдемся по карте где записаны части тела с boxecomponent и вытащим их положение и размер
	for (auto Pair : MultiplayerCharacter->HitCollisionBoxes)
	{
		FBoxInformation BoxInformation;
		BoxInformation.Location = Pair.Value->GetComponentLocation();
		BoxInformation.Rotation = Pair.Value->GetComponentRotation();
		BoxInformation.BoxEntent = Pair.Value->GetScaledBoxExtent();
		//27.4 положим в FramePackage информацию о коробке
		FramePackage.HitBoxInfo.Add(Pair.Key, BoxInformation);
	}
}

void ULagCompensationComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

void ULagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                              FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);


}

