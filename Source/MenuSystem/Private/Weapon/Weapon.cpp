// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Weapon.h"

#include "MeshPassProcessor.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Character/MultiplayerCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "HUD/MultiplayerHUD.h"
#include "Kismet/KismetMathLibrary.h"
#include "MenuSystem/MenuSystem.h"
#include "MultiplayerComponent/CombatComponent.h"
#include "Net/UnrealNetwork.h"
#include "Player/MultiplayerPlayerController.h"
#include "Weapon/Casing.h"


AWeapon::AWeapon()
{	// репликация
	bReplicates = true;
	
	PrimaryActorTick.bCanEverTick = false;
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(GetRootComponent());
	//9.1 сделаем движение реплицированным, что если он упадет то туда где у сервера
	SetReplicateMovement(true);
	
	SetRootComponent(WeaponMesh);
	
	WeaponMesh->SetCollisionResponseToAllChannels(ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// сфера для стокновений и overlap
	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(RootComponent);
	AreaSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PickUpWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickUpWidgetComponent"));
	PickUpWidgetComponent->SetupAttachment(RootComponent);
	PickUpWidgetComponent->SetVisibility(false);

	NiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>("NiagaraComponent");
	NiagaraComponent->SetupAttachment(RootComponent);
	//19.1 добавим подсветку предмета с помощью PostProcessVolume и материала добавленный в него
	WeaponMesh->SetRenderCustomDepth(true);
	WeaponMesh->CustomDepthStencilValue = CUSTOM_DEPTH_BLUE;
	WeaponMesh->MarkRenderStateDirty();
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();
	// для того чтобы выполнялось на сервере HasAuthority, включим оверлам на пешку
	if ( WeaponState !=EWeaponState::EWC_Equipped)
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		AreaSphere->SetCollisionResponseToChannel(ECC_Pawn,ECR_Overlap);
		// добавим begin overlap чтобы показывать виджет
		AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnSphereBeginOverlap);
		AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AWeapon::OnSphereEndOverlap);
	}
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AWeapon::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeapon,WeaponState);
	//DOREPLIFETIME(AWeapon,WeaponAmmo);
}


FHUDPackage AWeapon::GetHudPackage() const
{
	FHUDPackage HUDPackage;
	HUDPackage.CrosshairsCenter = CrosshairsCenter;
	HUDPackage.CrosshairsTop = CrosshairsTop;
	HUDPackage.CrosshairsBottom = CrosshairsBottom;
	HUDPackage.CrosshairsLeft = CrosshairsLeft;
	HUDPackage.CrosshairsRight = CrosshairsRight;
	HUDPackage.MaxSpreadMagnitude = MaxSpreadMagnitude;
	return HUDPackage;
}

void AWeapon::Dropped()
{
	// создадим правило отцепления, мировые координаты будут
	FDetachmentTransformRules DetachRules = FDetachmentTransformRules(EDetachmentRule::KeepWorld,true);
	// отцепим от актера
	DetachFromActor(DetachRules);
	// переключим состояние на Dropped где мы включим физику и коллизии
	SetWeaponState(EWeaponState::EWC_Dropped);
	// занулим владельца
	SetOwner(nullptr);
	MultiplayerCharacter = nullptr;
	MultiplayerPlayerController = nullptr;
	if (bIsStandardWeapon)
	{
		SetLifeSpan(1.f);
	}
}

void AWeapon::SetHUDAmmo_Public()
{
	SetHUDAmmo();
}

void AWeapon::OnRep_Owner()
{
	Super::OnRep_Owner();
	AMultiplayerCharacter* Character =  Cast<AMultiplayerCharacter>(GetOwner());
	if (Character)
	{	// проверим что оружие относиться к основному а не дполнительному, иначе не обновим HUD
		if (Character->GetCombatComponent()->GetWeapon() == this)
		{// обновим у клиента HUD на всякий если вдруг не успело сработать у клиента до SetOwner
			SetHUDAmmo();
		}
	}
}

void AWeapon::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                                   bool bFromSweep, const FHitResult& SweepResult)
{
	AMultiplayerCharacter* Character =  Cast<AMultiplayerCharacter>(OtherActor);
	/*FVector PawnLocation = UGameplayStatics::GetPlayerController(this,0)->GetPawn()->GetActorLocation();
	//FVector WeaponLocation = GetActorLocation();
	// DeltaDistance = (PawnLocation - GetActorLocation()).Length();
	//bool IsNeedToActivate = DeltaDistance <= 80.f;*/
	
	if (Character && PickUpWidgetComponent && WeaponState != EWeaponState::EWC_Equipped)
	{
		//PickUpWidgetComponent->SetVisibility(true);
		// передадим указатель оружия персонажу
		Character->SetOverlappingWeapon(this);
	}
}

void AWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AMultiplayerCharacter* Character =  Cast<AMultiplayerCharacter>(OtherActor);
	// когда уходит из сферы передаем нулевой указатель и выключаем 
	if (Character && PickUpWidgetComponent )
	{
		Character->SetOverlappingWeapon(nullptr);
	}
}

void AWeapon::SetWeaponState(EWeaponState InWeaponState)
{
	// поменяем состояние 
	WeaponState = InWeaponState;
	// и проверим у сервера получается, какое состояние и отключим overlap у AreaSphere
	OnWeaponState();
}

void AWeapon::OnRep_WeaponState()
{	// и проверим у клиента, какое состояние и отключим overlap у AreaSphere
	switch (WeaponState)
	{
	case EWeaponState::EWC_Equipped:
		OnEquippedState();
		break;
		// если оружие должно упасть при смерти то включим физику и коллизии
	case EWeaponState::EWC_Dropped:
		OnDroppedState();
		break;
	case EWeaponState::EWC_SecondaryEquipped:
		OnSecondaryEquipped();
		break;
	}
}

void AWeapon::OnWeaponState()
{
	switch (WeaponState)
	{
	case EWeaponState::EWC_Equipped:
		OnEquippedState();
		break;
		
	case EWeaponState::EWC_Dropped:
		OnDroppedState();
		break;
	
	case EWeaponState::EWC_SecondaryEquipped:
	OnSecondaryEquipped();
	break;
	}
}

void AWeapon::OnSecondaryEquipped()
{
	PickUpWidgetComponent->SetVisibility(false);
	GetWeaponMesh()->SetSimulatePhysics(false);
	GetWeaponMesh()->SetEnableGravity(false);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetWeaponMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	//13.1 Добавим включение физики для узи, чтобы ремешок его работал
	if (WeaponType == EWeaponType::EWT_SMG)
	{
		GetWeaponMesh()->SetCollisionResponseToAllChannels(ECR_Ignore);
		GetWeaponMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		GetWeaponMesh()->SetEnableGravity(true);
	}
	// 19.3 выключим подсветку т.к оружие в руках
	SetRenderCustomDepth(false);
}

void AWeapon::OnEquippedState()
{
	PickUpWidgetComponent->SetVisibility(false);
	GetWeaponMesh()->SetSimulatePhysics(false);
	GetWeaponMesh()->SetEnableGravity(false);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetWeaponMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	//13.1 Добавим включение физики для узи, чтобы ремешок его работал
	if (WeaponType == EWeaponType::EWT_SMG)
	{
		GetWeaponMesh()->SetCollisionResponseToAllChannels(ECR_Ignore);
		GetWeaponMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		GetWeaponMesh()->SetEnableGravity(true);
	}
	// 19.3 выключим подсветку т.к оружие в руках
	SetRenderCustomDepth(false);
}
void AWeapon::OnDroppedState()
{
	if (HasAuthority())
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	GetWeaponMesh()->SetSimulatePhysics(true);
	GetWeaponMesh()->SetEnableGravity(true);
	GetWeaponMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	//13.2 Добавим 2 строки чтобы потом узи тоже после dropped реагировала на все кроме игроков
	GetWeaponMesh()->SetCollisionResponseToAllChannels(ECR_Block);
	GetWeaponMesh()->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);
	// 19.4 включим подсветку т.к оружие уже не в руках
	SetRenderCustomDepth(true);
}

//19.2 включение/выключение влияние CustomDepth постпроцесса на оружие
void AWeapon::SetRenderCustomDepth(bool bSetEnabled)
{
	WeaponMesh->SetRenderCustomDepth(bSetEnabled);
}

void AWeapon::PlayFireEffect_Implementation()
{
	// проверяем что есть ассет и если есть то спавним эффект
	if (!NiagaraFireEffect)
	{
		NiagaraFireEffect = NiagaraComponent->GetAsset();
	}
	if (NiagaraFireEffect)
	{	
		if (!IsValid(FireEffect))
		{
			FireEffect = UNiagaraFunctionLibrary::SpawnSystemAttached(NiagaraFireEffect,GetRootComponent(),
			   SocketNameOnWeapon,FVector::ZeroVector,FRotator::ZeroRotator,
			   EAttachLocation::KeepRelativeOffset,true,false);
			FireEffect->Activate();
		}
	}
}

void AWeapon::StopFireEffect_Implementation()
{
	if (FireEffect && FireEffect->GetOwner())
	{
		FireEffect->DestroyComponent();
	}
}

void AWeapon::OnRep_WeaponAmmo()
{
	SetHUDAmmo();
	if (WeaponType == EWeaponType::EWT_Shotgun)
	{	//18.1 проверим патроны у дробовика и если они полные отправимся в последний кусок анимации
		if (IsFullAmmo())
		{
			if (MultiplayerCharacter &&
			MultiplayerCharacter->GetMesh())
			{
				MultiplayerCharacter->GetMesh()->GetAnimInstance()->Montage_JumpToSection("ShotgunReloadingFinished");
			}
		}
	}
}

void AWeapon::OpenFire(const FVector_NetQuantize& TargetPoint)
{
	if (CasingClass)
	{
		UWorld* World = GetWorld();
		FTransform SocketCasingTransform = GetWeaponMesh()->GetSocketTransform(SocketNameForCasing,RTS_World);
		
		//FTransform SocketTransform = GetWeaponMesh()->GetSocketTransform(SocketNameOnWeapon,RTS_World);
		APawn* WeaponInstigator = Cast<APawn>(GetOwner());
	
		if (SocketCasingTransform.IsValid() )
		{
			FActorSpawnParameters SpawnParams;
			// заспавним гильзу
			World->SpawnActor<ACasing>(CasingClass, SocketCasingTransform.GetLocation(),SocketCasingTransform.Rotator(), SpawnParams);
		}	
	}
	// функция вычитания боеприпаса и показа его в Overlay.
	SpendAmmo();
	
}

void AWeapon::SpendAmmo()
{
	SumAddChange -= 1;
	// уменьшим кол-во патронов 
	WeaponAmmo = FMath::Clamp(WeaponAmmo - 1, 0, MaxWeaponAmmo);
	// обновим Overlay
	SetHUDAmmo();
	if (HasAuthority())
	{
		ClientSpendAmmo(WeaponAmmo);
	}
	else
	{
		++SumSpendChange;
	}
}

void AWeapon::ClientSpendAmmo_Implementation(int32 ServerAmmo)
{
	if (HasAuthority()) return;
	WeaponAmmo = ServerAmmo;
	--SumSpendChange;
	WeaponAmmo -= SumSpendChange;
	SetHUDAmmo();
}

void AWeapon::SetCurrentAmmo(const int32 Ammo)
{	
	// уменьшим кол-во патронов 
	WeaponAmmo = FMath::Clamp(WeaponAmmo = Ammo, 0, MaxWeaponAmmo);
	// обновим Overlay
	SetHUDAmmo();
	if (HasAuthority())
	{
		ClientSetAmmo(WeaponAmmo);
	}
}

void AWeapon::ClientSetAmmo_Implementation(int32 ServerAmmo)
{
	if (HasAuthority()) return;
	// уменьшим кол-во патронов 
	WeaponAmmo = FMath::Clamp(WeaponAmmo = ServerAmmo, 0, MaxWeaponAmmo);
	
	// обновим Overlay
	SetHUDAmmo();
}

void AWeapon::ClientAddAmmo_Implementation(int32 ServerAmmo, int32 ChangeAmount)
{
	// НЕРАБОТАЕТ И НЕ ИСпользуется
	if (HasAuthority()) return;
	//WeaponAmmo += InAmmo;
	
	SumAddChange = SumAddChange - (ServerAmmo - WeaponAmmo);
	WeaponAmmo = ServerAmmo;
	WeaponAmmo -= SumAddChange;
	/*
	if (InAmmo > 0)
	{
		SumChange -= InAmmo;
		WeaponAmmo -= SumChange;
	}
	else
	{
		SumChange += InAmmo;
		WeaponAmmo += SumChange;
	}*/
	SetHUDAmmo();
}



void AWeapon::SetHUDAmmo()
{
	if (GetOwner())
	{
		// получим указатели на персонажа и на контроллер
		MultiplayerCharacter = MultiplayerCharacter ==nullptr ? Cast<AMultiplayerCharacter>(GetOwner()) : MultiplayerCharacter;
		MultiplayerPlayerController = MultiplayerPlayerController == nullptr ? Cast<AMultiplayerPlayerController>(MultiplayerCharacter->GetController()) : MultiplayerPlayerController;
	
		if (MultiplayerPlayerController)
		{	// обновим значение в виджете
			MultiplayerPlayerController->SetHUDWeaponAmmo(WeaponAmmo);
		}
	}
}



bool AWeapon::IsEmpty()
{
	if (WeaponAmmo <= 0)
	{
		return true;
	}
	return false;
}

bool AWeapon::IsFullAmmo()
{
	return WeaponAmmo >= MaxWeaponAmmo;
}

void AWeapon::ShowPickUpWidget(bool bVisibilityWidget)
{
	PickUpWidgetComponent->SetVisibility(bVisibilityWidget);
}

void AWeapon::SetbStandardWeapon(const bool InStandardWeapon)
{
	bIsStandardWeapon = InStandardWeapon;
}

FVector AWeapon::TraceEndWithScatters(const FVector& HitTarget)
{
	if (!GetWeaponMesh()->GetSocketByName(SocketNameOnWeapon)) return FVector::ZeroVector;
	
	const FTransform SocketTransform = GetWeaponMesh()->GetSocketTransform(SocketNameOnWeapon);
	//10.1 получим начальную точку для HeatScan
	FVector TraceStart = SocketTransform.GetLocation();
	// 13.1 получим вектор единичный от ствола оружие до цели трассировки
	FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	// 13.2 Найдем центр сферы разлета пуль или дробинок
	FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;
	// 13.3 нарисуем сферу разлета
	DrawDebugSphere(GetWorld(),SphereCenter,RadiusSphereScatter,12,FColor::Red,false,5.f);
	// 13.4 Возьмем рандомный в направление вектор и умножим его рандомную величину от 0 до радиуса сферы разлета
	FVector RandomVector = UKismetMathLibrary::RandomUnitVector() * FMath::RandRange(0.f,RadiusSphereScatter);
	// 13.5 прибавим рандом и центр сферы 
	FVector ScatterVector = SphereCenter + RandomVector;
	// 13.7 направление от начало ствола до разлетного вектора
	FVector ToScatterVector = ScatterVector - TraceStart;
	
	// 13.7 нарисуем сферу в месте прибавление рандома
	DrawDebugSphere(GetWorld(),SphereCenter + RandomVector,8.f,12,FColor::Orange,false,5.f);
	//и далее чтобы он был длиннее для трассировки умножим TRACE_LENGHT, но чтобы числа не большие были поделим на длину начального вектора
	DrawDebugLine(GetWorld(),TraceStart,TraceStart + ToScatterVector * TRACE_LENGHT/ToScatterVector.Size(),FColor::Cyan,false, 5.f);
	
	return FVector(TraceStart + ToScatterVector * TRACE_LENGHT / ToScatterVector.Size());
}

void AWeapon::TraceEndWithScattersForShotgun(const FVector& HitTarget, TArray<FVector_NetQuantize>& ShotgunHits)
{
	if (!GetWeaponMesh()->GetSocketByName(SocketNameOnWeapon)) return;
	
	const FTransform SocketTransform = GetWeaponMesh()->GetSocketTransform(SocketNameOnWeapon);
	//10.1 получим начальную точку для HeatScan
	FVector TraceStart = SocketTransform.GetLocation();
	// 13.1 получим вектор единичный от ствола оружие до цели трассировки
	FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	// 13.2 Найдем центр сферы разлета пуль или дробинок
	FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;
	// 13.3 нарисуем сферу разлета
	DrawDebugSphere(GetWorld(),SphereCenter,RadiusSphereScatter,12,FColor::Red,false,5.f);
	for (int i = 0; i < CountOfPellets; i++)
	{
		// 13.4 Возьмем рандомный в направление вектор и умножим его рандомную величину от 0 до радиуса сферы разлета
		FVector RandomVector = UKismetMathLibrary::RandomUnitVector() * FMath::RandRange(0.f,RadiusSphereScatter);
		// 13.5 прибавим рандом и центр сферы 
		FVector ScatterVector = SphereCenter + RandomVector;
		// 13.7 направление от начало ствола до разлетного вектора
		FVector ToScatterVector = ScatterVector - TraceStart;
	
		// 13.7 нарисуем сферу в месте прибавление рандома
		DrawDebugSphere(GetWorld(),SphereCenter + RandomVector,8.f,12,FColor::Orange,false,5.f);
		//и далее чтобы он был длиннее для трассировки умножим TRACE_LENGHT, но чтобы числа не большие были поделим на длину начального вектора
		DrawDebugLine(GetWorld(),TraceStart,TraceStart + ToScatterVector * TRACE_LENGHT/ToScatterVector.Size(),FColor::Cyan,false, 5.f);
	
		ShotgunHits.Add(TraceStart + ToScatterVector * TRACE_LENGHT / ToScatterVector.Size());
	}
	
}























