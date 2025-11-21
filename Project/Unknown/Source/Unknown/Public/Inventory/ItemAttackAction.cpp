#include "Inventory/ItemAttackAction.h"
#include "GameFramework/Character.h"

UItemAttackAction::UItemAttackAction()
{
	bIsOnCooldown = false;
	AttackRange = 250.0f;
}

void UItemAttackAction::OnAttackAnimationFinished()
{
	bIsOnCooldown = false;
}

