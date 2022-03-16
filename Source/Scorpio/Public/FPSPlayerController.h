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
};
