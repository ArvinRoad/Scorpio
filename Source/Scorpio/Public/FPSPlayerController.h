#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FPSPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class SCORPIO_API AFPSPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	/* 屏幕抖动 */
	void PlayerCameraShake(TSubclassOf<UCameraShakeBase> CameraShake);

	/* Game Player UI */
	UFUNCTION(BlueprintImplementableEvent,Category="PlayerUI")
	void CreatePlayerUI();

	/* 十字线UI扩散动画 */
	UFUNCTION(BlueprintImplementableEvent,Category="PlayerUI")
	void DoCrosshairRecoil();

	/* 弹药更新 */
	UFUNCTION(BlueprintImplementableEvent,Category="PlayerUI")
	void UpdateAmmoUI(int32 ClipCurrentAmmo,int32 GunCurrentAmmo);

	/* 玩家血量更新 */
	UFUNCTION(BlueprintImplementableEvent,Category="PlayerUI")
	void UpdateHealthUI(float NewHealth);

	UFUNCTION(BlueprintImplementableEvent,Category="Health")
	void DeathMatchDeath(AActor* DamageCauser);
};
