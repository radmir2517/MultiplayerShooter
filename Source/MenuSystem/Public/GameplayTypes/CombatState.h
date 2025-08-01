#pragma once

// боеовое состояния игрока
UENUM(BlueprintType)
enum class ECombatState : uint8
{
	ECT_Unoccupied UMETA(DisplayName = "Unoccupied"),
	ECT_Reloading UMETA(DisplayName = "Reloading"),
	ECT_ThrowGrenade UMETA(DisplayName = "ThrowGrenade"),
	ECT_MAX UMETA(DisplayName = "Default Max")
};
