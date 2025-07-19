// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Weapon.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Character/MultiplayerCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "HUD/MultiplayerHUD.h"
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
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();
	// для того чтобы выполнялось на сервере HasAuthority, включим оверлам на пешку
	if (HasAuthority() && WeaponState !=EWeaponState::EWC_Equipped)
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
	DOREPLIFETIME(AWeapon,WeaponAmmo);
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
}

void AWeapon::SetHUDAmmo_Public()
{
	SetHUDAmmo();
}


void AWeapon::OnRep_Owner()
{
	Super::OnRep_Owner();
	// обновим у клиента HUD на всякий если вдруг не успело сработать у клиента до SetOwner
	SetHUDAmmo();
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
{	// поменяем состояние 
	WeaponState = InWeaponState;
	// и проверим у сервера получается, какое состояние и отключим overlap у AreaSphere
	switch (WeaponState)
	{
	case EWeaponState::EWC_Equipped:
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
		break;
		
	case EWeaponState::EWC_Dropped:
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
		break;
	}
}

void AWeapon::OnRep_WeaponState()
{	// и проверим у клиента, какое состояние и отключим overlap у AreaSphere
	switch (WeaponState)
	{
	case EWeaponState::EWC_Equipped:
		PickUpWidgetComponent->SetVisibility(false);
		GetWeaponMesh()->SetSimulatePhysics(false);
		GetWeaponMesh()->SetEnableGravity(false);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GetWeaponMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
		// если оружие должно упасть при смерти то включим физику и коллизии
	case EWeaponState::EWC_Dropped:
		GetWeaponMesh()->SetSimulatePhysics(true);
		GetWeaponMesh()->SetEnableGravity(true);
		GetWeaponMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		break;
	}
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
}


void AWeapon::OpenFire(const FVector_NetQuantize& TargetPoint)
{
	if (CasingClass)
	{
		UWorld* World =  GetWorld();
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
	// уменьшим кол-во патронов 
	WeaponAmmo = FMath::Clamp(WeaponAmmo - 1, 0, MaxWeaponAmmo);
	// обновим Overlay
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

void AWeapon::ShowPickUpWidget(bool bVisibilityWidget)
{
	PickUpWidgetComponent->SetVisibility(bVisibilityWidget);
}




























