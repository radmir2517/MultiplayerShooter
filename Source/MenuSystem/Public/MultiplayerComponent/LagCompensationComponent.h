// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LagCompensationComponent.generated.h"

class AWeapon;
class AMultiplayerPlayerController;
class AMultiplayerCharacter;

//26.2 Информация о локации, вращения и размеров каждой чаcти бокса тела
USTRUCT(BlueprintType)
struct FBoxInformation
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Location;

	UPROPERTY()
	FRotator Rotation;

	UPROPERTY()
	FVector BoxEntent;
	
};
// 26.1 Добавим структуру которая будет хранить время и карту с информации
// о BoxCollision каждой части тела которую мы добавили
USTRUCT(BlueprintType)
struct FFramePackage
{
	GENERATED_BODY()
	
	UPROPERTY()
	float Time;

	UPROPERTY()
	TMap<FName, FBoxInformation> HitBoxInfo;
	// 33.6 добавим сюда персонажа чтобы различать в дробовиках в кого конкретно попали
	UPROPERTY()
	AMultiplayerCharacter* Character;
};

USTRUCT(BlueprintType)
struct FServerSideRewindResult
{
	GENERATED_BODY()
	
	bool bWasHitting = false;

	bool bHeadShot = false;
};

USTRUCT(BlueprintType)
struct FServerShotgunSideRewindResult
{//33,1 структуру в которой будем хранить как в дробовиках Tmap в котором будет  попадания в голову и кол-во и также в тело и кол-во
	GENERATED_BODY()

	UPROPERTY()
	TMap<AMultiplayerCharacter*, int32> HitOnHeadCharacters;
	UPROPERTY()
	TMap<AMultiplayerCharacter*, int32> HitOnBodyCharacters;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MENUSYSTEM_API ULagCompensationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	ULagCompensationComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;
	//26.4.1 Сеттеры для контроллера и персонажа
	FORCEINLINE void SetMultiplayerCharacter(AMultiplayerCharacter* Character) {MultiplayerCharacter = Character;}
	FORCEINLINE void SetMultiplayerPlayerController(AMultiplayerPlayerController* PlayerController) {MultiplayerController = PlayerController;}
	//26.4.2 Геттеры для контроллера и персонажа
	FORCEINLINE AMultiplayerCharacter* GetMultiplayerCharacter() {return MultiplayerCharacter;}
	FORCEINLINE AMultiplayerPlayerController* GetMultiplayerPlayerController() {return MultiplayerController;}
	FORCEINLINE TDoubleLinkedList<FFramePackage>& GetFrameHistory () {return FrameHistory;}
	
	//27.5 Созаддим функцию которая будет показывать визуально как расположены коробки
	void ShowFramePackage(const FFramePackage& FramePackage, const FColor& Color);
	//27.1 Создадим функцию которая будет сохранять позицию boxes частей тела и время
	void SafeFramePackage(FFramePackage& FramePackage);
	//30.1 функция интерполяция, мы переместим персонажа в нужный момент времени между готовыми фреймами и найдем точное местоположение
	FFramePackage InterpBetweenFrames(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, float HitTime);
	
	// 29,1 функция которая будет вычислять момент в списке который нужно вытащить
	FServerSideRewindResult ServerSideRewind(const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation,
	AMultiplayerCharacter* HitCharacter, float HitTime);
	
	//31.1 Одна из главнхы функции, которая перемещает коробки и делает трассировку выстрела на предмет попадания
	FServerSideRewindResult ConfirmShot(FFramePackage& Package, AMultiplayerCharacter* HitCharacter, FVector_NetQuantize StartTrace, FVector_NetQuantize EndTrace);
	// 31.2 Получим текущее положение коробок и запомним в отдельный фрейм
	void CacheBoxPositions(AMultiplayerCharacter* HitCharacter, FFramePackage& FramePackage);
	// 31,3 Переместим коробки в заданное время врейма
	void MoveBoxes(AMultiplayerCharacter* HitCharacter, FFramePackage& FramePackage);
	// 31,4 в конце переместим коробки обратно
	void ResetHitBoxes(AMultiplayerCharacter* HitCHracter, FFramePackage& FramePackage);
	// 31,5 вкл/выкл колизии мешки чтобы не мешался трассировки
	void EnableCharacterMeshCollision(AMultiplayerCharacter* HitChracter, ECollisionEnabled::Type CollisionEnabled);
	
	//32.1 Функция которая будет перематывать время и смотреть было ли попадания и наносить урон если было
	UFUNCTION(Server, Reliable)
	void ServerScoreRequest(AMultiplayerCharacter* HitCharacter,const FVector_NetQuantize& TraceStart,const FVector_NetQuantize& HitLocation, float HitTime,AWeapon* Weapon);
	
	FFramePackage GetFrameToCheck(AMultiplayerCharacter* HitCharacter, float HitTime);

	/*
	 * Shotgun
	 */

	// 33,2 функция которая будет вычислять момент в списке который нужно вытащить
	FServerShotgunSideRewindResult ShotgunServerSideRewind(
		const FVector_NetQuantize& TraceStart,
		const TArray<FVector_NetQuantize>& HitLocations,
	TArray<AMultiplayerCharacter*>& HitCharacters,
	float HitTime);
	
	FServerShotgunSideRewindResult ConfirmShotgunShot(
		const TArray<FVector_NetQuantize>& HitLocations,
		const FVector_NetQuantize& TraceStart,
		const TArray<FFramePackage>& FramePackages);
	
protected:
	virtual void BeginPlay() override;
	
	//32.4 фукция котоаря каждый кадр записывает фрейма с положением коробок персонажа в сервере
	void SaveFramePackage();
	
	// 26.3 Указатели о персонаже и его контроллере
	UPROPERTY()
	AMultiplayerCharacter* MultiplayerCharacter;
	UPROPERTY()
	AMultiplayerPlayerController* MultiplayerController;

	// 28.3 время которое мы будем хранить игрока
	UPROPERTY(EditDefaultsOnly)
	float MaxTimeFrameHistory = 4.f;
	// 28.2 создадим двухсвязный список который знает во первых кто первый элемент и последний
	// а каждый элемент знает предидущего и следующего
	TDoubleLinkedList<FFramePackage> FrameHistory;
	
};














































