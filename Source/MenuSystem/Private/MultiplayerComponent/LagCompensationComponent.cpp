// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerComponent/LagCompensationComponent.h"

#include "Character/MultiplayerCharacter.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Weapon/Weapon.h"


ULagCompensationComponent::ULagCompensationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}

void ULagCompensationComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

void ULagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType,
											  FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	//32.7 Свернем в функцию, сохранение персонажа с коробками каждый кадр
	SaveFramePackage();
}


void ULagCompensationComponent::SaveFramePackage()
{
	if (MultiplayerCharacter == nullptr || !MultiplayerCharacter->HasAuthority()) return;
	// 28,3 Теперь мы будеи использоваться двух связный список
	// проверим если он пустой или мало элементов то просто добавим новый frame с расположение квадратов
	if (FrameHistory.Num() <= 1)
	{	// добавил в верзуюю часть наш фрейм
		FFramePackage ThisFrame;
		SafeFramePackage(ThisFrame);
		FrameHistory.AddHead(ThisFrame);
		//ShowFramePackage(ThisFrame,FColor::Red);
	}
	else
	{
		// проверим не превышаем ли лимит времени в 4 сек если превышаем удаляем по одному с конца списка
		float DeltaTime = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
		while (DeltaTime > MaxTimeFrameHistory)
		{
			FrameHistory.RemoveNode(FrameHistory.GetTail());
			DeltaTime = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
		}
		// потом добавим текущий фрейм
		FFramePackage ThisFrame;
		SafeFramePackage(ThisFrame);
		FrameHistory.AddHead(ThisFrame);
		// и покажем его
		//ShowFramePackage(ThisFrame,FColor::Red);
	}
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
			false,
			MaxTimeFrameHistory);
	}
}

void ULagCompensationComponent::SafeFramePackage(FFramePackage& FramePackage)
{
	if (!MultiplayerCharacter) MultiplayerCharacter = Cast<AMultiplayerCharacter>(GetOwner());
	if (!MultiplayerCharacter) return;
	// 27.2 получим время
	FramePackage.Time = GetWorld()->GetTimeSeconds();
	FramePackage.Character = MultiplayerCharacter;
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

FFramePackage ULagCompensationComponent::InterpBetweenFrames(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame,
	float HitTime)
{
	//30.1 вся жистанция во времени между 2 фреймами
	float Distance = YoungerFrame.Time - OlderFrame.Time;
	// найдем разницу между нашим временем и старым кадром и поделем на весь путь это будет как скорость InterpSpeed
	float InterpFraction = FMath::Clamp<float>(  ((HitTime - OlderFrame.Time) / Distance), 0.f,1.f );
	// Создадим новый фрейм
	FFramePackage InterpFrame;
	InterpFrame.Time = HitTime;
	InterpFrame.Character = YoungerFrame.Character;
	// пройдемся по всем HitBox в старом и новом фрейме
	for (auto Pair : YoungerFrame.HitBoxInfo)
	{
		FBoxInformation BoxInformation;
		// и сделаем интерполяцию между между старым и новым но, скорость мы вычислили там где у нас равна моменту было попадание.
		// т.е интерполяция завершится ровна на этом моменте
		BoxInformation.Location = FMath::VInterpTo(OlderFrame.HitBoxInfo[Pair.Key].Location, Pair.Value.Location,1.f, InterpFraction);
		BoxInformation.Rotation = FMath::RInterpTo(OlderFrame.HitBoxInfo[Pair.Key].Rotation, Pair.Value.Rotation, 1.f, InterpFraction);
		BoxInformation.BoxEntent = Pair.Value.BoxEntent;
		// добавим в карту hitboxinfo интерполириванную коробку
		InterpFrame.HitBoxInfo.Add(Pair.Key, BoxInformation);
	}
	
	return InterpFrame;
}

FFramePackage ULagCompensationComponent::GetFrameToCheck(AMultiplayerCharacter* HitCharacter, float HitTime)
{
	bool bReturn = HitCharacter == nullptr ||
		HitCharacter->GetLagCompensationComponent() == nullptr ||
		HitCharacter->GetLagCompensationComponent()->GetFrameHistory().GetHead() == nullptr ||
		HitCharacter->GetLagCompensationComponent()->GetFrameHistory().GetTail() == nullptr;
	if (bReturn) return FFramePackage();
	//29.2 просто ссылка для удоства
	TDoubleLinkedList<FFramePackage>& HitCharacterFrameHistory = HitCharacter->GetLagCompensationComponent()->GetFrameHistory();

	bool bNeedToInterpolite = true;
	//29.3 время самого первого и последнего(свежего) пакета
	float YoungerTime = HitCharacterFrameHistory.GetHead()->GetValue().Time;
	float OlderTime = HitCharacterFrameHistory.GetTail()->GetValue().Time;
	FFramePackage NecessaryPackage;

	//29.4 сначало начнем сравнивать крайние значения времени время
	if (OlderTime > HitTime)
	{ //29.5 время цели слишком далеко от перемотки
		return FFramePackage();
	}
	if (YoungerTime < HitTime || YoungerTime == HitTime)
	{//29.6 время больше чем самое свежие у нас значит это был сервер
		NecessaryPackage = HitCharacterFrameHistory.GetHead()->GetValue();
		bNeedToInterpolite = false;
	}
	//29.7 теперь начнем пробегаться по истории
	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* OldestNode = HitCharacterFrameHistory.GetHead();
	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* YoungerNode = OldestNode;
	
	while (OldestNode->GetValue().Time > HitTime)
	{//29.8 если следущий будет пустой то остановим
		if (OldestNode->GetNextNode() == nullptr)
		{
			break;
		}
		
		if (OldestNode->GetValue().Time < HitTime)
		{ //29.9 мы нашли точку когда наша время стало меньше требуемого
			break;
		}
		if(OldestNode->GetValue().Time == HitTime)
		{//29.10 редкий случай когда он может быть равен
			NecessaryPackage = OldestNode->GetValue();
			bNeedToInterpolite = false;
			break;
		}
		//29.11 если не сработало то идем дальше
		OldestNode = OldestNode->GetNextNode();
		if (OldestNode->GetValue().Time > HitTime)
		{
			YoungerNode = OldestNode;
		}
	}
	// while прошел теперь пора интерполировать
	if (bNeedToInterpolite)
	{
		return NecessaryPackage = InterpBetweenFrames(OldestNode->GetValue(),YoungerNode->GetValue(),HitTime);
	}
	
	return FFramePackage();
}

FServerShotgunSideRewindResult ULagCompensationComponent::ShotgunServerSideRewind(const FVector_NetQuantize& TraceStart,
	const TArray<FVector_NetQuantize>& HitLocations, TArray<AMultiplayerCharacter*>& HitCharacters, float HitTime)
{
	//33.3 создадим массив фреймов
	TArray<FFramePackage> FramePackages;
	// 33.4 пройдемся по всем персонажам и получим нужный фрейм каждого
	for (AMultiplayerCharacter* HitCharacter: HitCharacters)
	{
		FramePackages.Add(GetFrameToCheck(HitCharacter, HitTime));
	}
	// 33,5 отправим на подтверждение выстрела 
	return ConfirmShotgunShot(HitLocations,TraceStart,FramePackages);
}

FServerShotgunSideRewindResult ULagCompensationComponent::ConfirmShotgunShot(
	const TArray<FVector_NetQuantize>& HitLocations, const FVector_NetQuantize& TraceStart,
	const TArray<FFramePackage>& FramePackages)
{
	return FServerShotgunSideRewindResult();
}

FServerSideRewindResult ULagCompensationComponent::ServerSideRewind(const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation,
                                                                    AMultiplayerCharacter* HitCharacter, float HitTime)
{
	FFramePackage NecessaryPackage = GetFrameToCheck(HitCharacter,HitTime);
	
	return ConfirmShot(NecessaryPackage,HitCharacter,TraceStart,HitLocation);
}

FServerSideRewindResult ULagCompensationComponent::ConfirmShot(FFramePackage& Package,
	AMultiplayerCharacter* HitCharacter, FVector_NetQuantize StartTrace, FVector_NetQuantize HitLocation)
{
	if (HitCharacter == nullptr || GetWorld()) return FServerSideRewindResult();
	// фрейм в котором мы будем хранить прям текущее положение коробок персонажа
	FFramePackage CurrentFramePackage;
	// получим положение коробок персонажа и запищем в фрейм
	CacheBoxPositions(HitCharacter, CurrentFramePackage);
	// переместим коробки теперь в то время когда был фрейм выстрела
	MoveBoxes(HitCharacter,Package);
	// отключим коллизию мешки чтобы не мешала
	EnableCharacterMeshCollision(HitCharacter,ECollisionEnabled::NoCollision);
	// включим только голову пока
	HitCharacter->HitCollisionBoxes["head"]->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HitCharacter->HitCollisionBoxes["head"]->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);

	// сделаем трассировку
	FHitResult ConfirmHitResult;
	UWorld* World = GetWorld();
	FVector_NetQuantize EndTrace = StartTrace + (HitLocation - StartTrace) * 1.25;
	World->LineTraceSingleByChannel(ConfirmHitResult,StartTrace,EndTrace,ECC_Visibility);

	// если есть попадаение в голову, столкновение с полом уберем позже
	if (ConfirmHitResult.bBlockingHit)
	{
		// отметим что есть попадание, вернем коробки назад и верним им выключенные коллизии
		ResetHitBoxes(HitCharacter, CurrentFramePackage);
		EnableCharacterMeshCollision(HitCharacter,ECollisionEnabled::QueryAndPhysics);
		return FServerSideRewindResult{true,true};
	}
	else
	{	// если нет попадания включаем коллизии остальных частей коробок
		for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes)
		{
			if (HitBoxPair.Value != nullptr)
			{
				HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				HitBoxPair.Value->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
			}
		}
		// делаем трассирвоку заново 
		World->LineTraceSingleByChannel(ConfirmHitResult,StartTrace,EndTrace,ECC_Visibility);
		if (ConfirmHitResult.bBlockingHit)
		{	// если есть то отметим это и вернем коробки где были, выключим их коллизии и вернем мешке коллизию
			ResetHitBoxes(HitCharacter, CurrentFramePackage);
			EnableCharacterMeshCollision(HitCharacter,ECollisionEnabled::QueryAndPhysics);
			return FServerSideRewindResult{true,false};
		}
	}

	// если ничего не было вернем коробки где были, выключим их коллизии и вернем мешке коллизию
	ResetHitBoxes(HitCharacter, CurrentFramePackage);
	EnableCharacterMeshCollision(HitCharacter,ECollisionEnabled::QueryAndPhysics);
	return FServerSideRewindResult{false,false};
}

void ULagCompensationComponent::CacheBoxPositions(AMultiplayerCharacter* HitCharacter, FFramePackage& FramePackage)
{
	for (auto& Pair : HitCharacter->HitCollisionBoxes)
	{
		FBoxInformation BoxInformation;
		
		BoxInformation.Location = Pair.Value->GetComponentLocation();
		BoxInformation.Rotation = Pair.Value->GetComponentRotation();
		BoxInformation.BoxEntent = Pair.Value->GetScaledBoxExtent();
		
		FramePackage.HitBoxInfo.Add(Pair.Key, BoxInformation);
	}
	
}

void ULagCompensationComponent::MoveBoxes(AMultiplayerCharacter* HitCharacter, FFramePackage& FramePackage)
{
	if (HitCharacter == nullptr) return;
	
	for (auto& Pair : FramePackage.HitBoxInfo)
	{
		if (HitCharacter->HitCollisionBoxes[Pair.Key] != nullptr)
		{
			HitCharacter->HitCollisionBoxes[Pair.Key]->SetWorldLocation(Pair.Value.Location);
			HitCharacter->HitCollisionBoxes[Pair.Key]->SetWorldRotation(Pair.Value.Rotation);
			HitCharacter->HitCollisionBoxes[Pair.Key]->SetBoxExtent(Pair.Value.BoxEntent);
		}
	}
}

void ULagCompensationComponent::ResetHitBoxes(AMultiplayerCharacter* HitCHracter, FFramePackage& FramePackage)
{
	if (HitCHracter == nullptr) return;
	
	for (auto& Pair : FramePackage.HitBoxInfo)
	{
		if (HitCHracter->HitCollisionBoxes[Pair.Key] != nullptr)
		{
			HitCHracter->HitCollisionBoxes[Pair.Key]->SetWorldLocation(Pair.Value.Location);
			HitCHracter->HitCollisionBoxes[Pair.Key]->SetWorldRotation(Pair.Value.Rotation);
			HitCHracter->HitCollisionBoxes[Pair.Key]->SetBoxExtent(Pair.Value.BoxEntent);
			HitCHracter->HitCollisionBoxes[Pair.Key]->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

void ULagCompensationComponent::EnableCharacterMeshCollision(AMultiplayerCharacter* HitChracter, ECollisionEnabled::Type CollisionEnabled)
{
	if (HitChracter ==nullptr ||HitChracter->GetMesh() ) return;

	HitChracter->GetMesh()->SetCollisionEnabled(CollisionEnabled);
}

void ULagCompensationComponent::ServerScoreRequest_Implementation(AMultiplayerCharacter* HitCharacter,const FVector_NetQuantize& TraceStart,const FVector_NetQuantize& HitLocation, float HitTime,AWeapon* Weapon)
{
	if (HitCharacter == nullptr || MultiplayerCharacter) return;
	// 32.2 перематаем персонажа на это время и проверим было ли попадание
	FServerSideRewindResult ShotResult = ServerSideRewind(TraceStart,HitLocation,HitCharacter,HitTime);
	// 32,3 если было то нанесем урон
	if (ShotResult.bWasHitting)
	{
		UGameplayStatics::ApplyDamage(HitCharacter,
			Weapon->GetHitScanDamage(),
			MultiplayerCharacter->Controller,
			Weapon,
			UDamageType::StaticClass());
	}
}























