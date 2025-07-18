#pragma once


UENUM(BlueprintType)
enum class TurningInPlace : uint8
{
	ETIP_Left UMETA(DisplayName = "TurningLeft"),
	ETIP_Right UMETA(DisplayName = "TurningRight"),
	ETIP_NotTurning UMETA(DisplayName = "NotTurning"),

	ETIP_MAX UMETA(DisplayName = "DefaultMax")
};
