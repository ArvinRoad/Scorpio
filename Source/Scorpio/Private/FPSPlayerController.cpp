#include "FPSPlayerController.h"

void AFPSPlayerController::PlayerCameraShake(TSubclassOf<UCameraShakeBase> CameraShake) {
	ClientPlayCameraShake(CameraShake,1,ECameraShakePlaySpace::CameraLocal,FRotator::ZeroRotator);
}
