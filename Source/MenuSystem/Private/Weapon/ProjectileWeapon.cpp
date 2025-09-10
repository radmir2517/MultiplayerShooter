// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileWeapon.h"

#include "Weapon/Projectile.h"

/*
FTransform AProjectileWeapon::CalculateSpawnBulletTransform(const FVector& TargetPoint)
{
	   // получим сокет на оружие
	   FTransform SocketTransform = GetWeaponMesh()->GetSocketTransform(SocketNameOnWeapon,RTS_World);
	   UWorld* World =  GetWorld();
	   FTransform SpawnBulletTransform;
	   
	   if (SocketTransform.IsValid() && World && ProjectileClass )
	   {	// из сокета оружия до ImpactPoint с середины экрана мы проведем вектор
		   FVector TargetDirection = TargetPoint - SocketTransform.GetLocation();
		   // и назначим их в Transform
		   FRotator BulletSpawnRotation = TargetDirection.Rotation();
		   FVector BulletSpawnLocation = SocketTransform.GetLocation();
		   SpawnBulletTransform.SetLocation(BulletSpawnLocation);
		   SpawnBulletTransform.SetRotation(BulletSpawnRotation.Quaternion());
	   }
	   return SpawnBulletTransform;
}
*/

AProjectileWeapon::AProjectileWeapon()
{
	FireType = EFireType::EFT_Projectile;
}

void AProjectileWeapon::OpenFire(const FVector_NetQuantize& TargetPoint)
{
	Super::OpenFire(TargetPoint);
	
	UWorld* World =  GetWorld();
	FRotator BulletSpawnRotation;
	FVector BulletSpawnLocation;
	FTransform SocketTransform = GetWeaponMesh()->GetSocketTransform(SocketNameOnWeapon,RTS_World);
	
	if (SocketTransform.IsValid() && World && ProjectileClass )
	{	// из сокета оружия до ImpactPoint с середины экрана мы проведем вектор
		FVector TargetDirection = TargetPoint - SocketTransform.GetLocation();
		// и назначим их в Transform
		BulletSpawnRotation = TargetDirection.Rotation();
		BulletSpawnLocation = SocketTransform.GetLocation();
	}
	
	//FTransform SocketTransform = GetWeaponMesh()->GetSocketTransform(SocketNameOnWeapon,RTS_World);
	APawn* WeaponInstigator = Cast<APawn>(GetOwner());
	
	if ( World && ProjectileClass && WeaponInstigator )
	{
		// назначим владельца и провокатора
		//FVector TargetDirection = InSpawnBulletTransform - SocketTransform.GetLocation();
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetOwner();
		SpawnParams.Instigator = WeaponInstigator;
		// заспавним пулю
		World->SpawnActor<AProjectile>(ProjectileClass, BulletSpawnLocation,BulletSpawnRotation, SpawnParams);
	}	
}
