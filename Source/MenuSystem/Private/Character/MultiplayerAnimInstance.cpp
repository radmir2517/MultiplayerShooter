// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/MultiplayerAnimInstance.h"

#include "Character/MultiplayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "MultiplayerComponent/CombatComponent.h"
#include "Weapon/Weapon.h"


void UMultiplayerAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	MultiplayerCharacter = Cast<AMultiplayerCharacter>(TryGetPawnOwner());
	
}

void UMultiplayerAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (MultiplayerCharacter == nullptr)
	{
		MultiplayerCharacter = Cast<AMultiplayerCharacter>(TryGetPawnOwner());
	}
	if (MultiplayerCharacter == nullptr ) return;
	
	FVector Velocity  =  MultiplayerCharacter->GetCharacterMovement()->Velocity;
	Velocity.Z = 0;
	Speed = Velocity.Size();

	bIsInAir = MultiplayerCharacter->GetCharacterMovement()->IsFalling();

	bIsAccelerating = MultiplayerCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0;

	bWeaponEquipped = IsValid(MultiplayerCharacter->GetWeapon());

	bCrouching =  MultiplayerCharacter->GetCharacterMovement()->IsCrouching();

	bAiming = MultiplayerCharacter->GetIsAiming();

	//
	// направления бега и анимации
	//

	// направление камеры
	FRotator AimRotation = MultiplayerCharacter->GetBaseAimRotation();
	// направление персонажа во время движения
	FRotator CharacterRotation = UKismetMathLibrary::MakeRotFromX(MultiplayerCharacter->GetVelocity());
	// делта ротаторов и вычленение из него поворота(Yaw), для задания анимации на BlendSpace
	FRotator NormalDeltaRot =  UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, AimRotation);
	// сделаем интепритацию для ротатора чтобы не было резкой смены
	DeltaRotation = FMath::RInterpTo(DeltaRotation,NormalDeltaRot,DeltaTime, 6.f);
	YawOffset = DeltaRotation.Yaw;
	
	//UE_LOG(LogTemp, Warning, TEXT("DeltaRotation: %f"), YawOffset);

	// для определения наклона персонажа во время начало изменения траектории вычслим последнее полочение в последнем кадре
	LastControlRotation = CurrentControlRotation;
	CurrentControlRotation = MultiplayerCharacter->GetActorRotation();
	// дельта ротатора между последним и предпоследнем кадре
	FRotator DeltaRotationLean = UKismetMathLibrary::NormalizedDeltaRotator(CurrentControlRotation,LastControlRotation);
	// усиливаем переменную, т.к это между кадрами то получим значение в одной секунде поделим на время кадра
	float YawDeltaRotation = DeltaRotationLean.Yaw / DeltaTime;
	// сделаем плавный переход между дельтами
	float InterpLean = FMath::FInterpTo(Lean, YawDeltaRotation, DeltaTime, 6.f);
	// ограничен в 90 градусов, больше нам не надо
	Lean = FMath::Clamp(InterpLean,-90.f,90.f);
	//UE_LOG(LogTemp, Warning, TEXT("Lean: %f"), Lean);

	//
	// AimOffset
	//
	AO_Yaw = MultiplayerCharacter->GetAO_Yaw();
	AO_Pitch = MultiplayerCharacter->GetAO_Pitch();

	// это для FABRIK IK, левой руки
	if (MultiplayerCharacter->GetWeapon() && MultiplayerCharacter->GetMesh())
	{
		// получим местоположение мировое у сокета
		LeftHandSocketTransform = MultiplayerCharacter->GetWeapon()->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), RTS_World);
		// выходные переменные
		FVector OutLocation;
		FRotator OutRotation;
		// преобразуем мировые координаты в относительные координаты сокета относительно правой руки, там оружие
		MultiplayerCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"),LeftHandSocketTransform.GetLocation(),FRotator::ZeroRotator,OutLocation,OutRotation);
		// установим эти относительные координаты для сокета
		LeftHandSocketTransform.SetLocation(OutLocation);
		LeftHandSocketTransform.SetRotation(OutRotation.Quaternion());

		//
		// Вращение кисти для правильного нарпавления оружия к точки стрельбы
		//
		if ( MultiplayerCharacter->IsLocallyControlled() && !bDisableGameplay)
		{
			bIsLocallyControlled = true;
			// получим трансформация кисти и получим место попадания
			RightHandTransform = MultiplayerCharacter->GetWeapon()->GetWeaponMesh()->GetSocketTransform("hand_r",RTS_World);
			FVector HitLocation = MultiplayerCharacter->GetHitLocation();
			
			// вычислим разницу в градусах между точкой попадания и текущим положением руки
			FRotator DeltaRotations = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(),  (RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - HitLocation)));
			RightHandRotation = FMath::RInterpTo(RightHandRotation,DeltaRotations,DeltaTime, 25.f); 
			
			FTransform MuzzleStart = MultiplayerCharacter->GetWeapon()->GetWeaponMesh()->GetSocketTransform("FireSocket");
			//DrawDebugLine(GetWorld(),MuzzleStart.GetLocation(),HitLocation,FColor::Blue,false,1.f);
			//DrawDebugLine(GetWorld(),MuzzleStart.GetLocation(),MuzzleStart.GetLocation() + FRotationMatrix(MuzzleStart.GetRotation().Rotator()).GetUnitAxis(EAxis::Y) * 1000.f,FColor::Green,false,1.f);
		}
		else
		{
			bIsLocallyControlled = false;
		}
	}
	// получение enum который меняется если AO_Yaw >90 || <-90
	TurningInPlace = MultiplayerCharacter->GetTurningInPlace();

	bRotateRootBone = MultiplayerCharacter->ShouldRotateRootBone();
	bIsCharacterEliminated = MultiplayerCharacter->IsCharacterEliminated();
	// для отключения Fabroc при перезардке
	bIsReloading = MultiplayerCharacter->GetCombatComponent()->CombatState == ECombatState::ECT_Reloading;
	bDisableGameplay = MultiplayerCharacter->IsDisabledGameplay();
}





































