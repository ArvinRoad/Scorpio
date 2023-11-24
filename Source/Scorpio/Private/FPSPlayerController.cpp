#include "FPSPlayerController.h"
#include "GameFramework/PlayerController.h"

void AFPSPlayerController::PlayerCameraShake(TSubclassOf<UCameraShakeBase> CameraShake) {

	/* 5.3 中 ClientPlayCameraShake 更名为 ClientStartCameraShake
	 *  ClientPlayCameraShake(CameraShake, 1, ECameraShakePlaySpace::CameraLocal, FRotator::ZeroRotator);
	 */

	ClientStartCameraShake(CameraShake, 1, ECameraShakePlaySpace::CameraLocal, FRotator::ZeroRotator);
}
