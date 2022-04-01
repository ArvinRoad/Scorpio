#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponBaseClien.generated.h"

UCLASS()
class SCORPIO_API AWeaponBaseClien : public AActor
{
	GENERATED_BODY()
	
public:	
	AWeaponBaseClien();

	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	USkeletalMeshComponent* WeaponMesh;

	/* 持有动画蒙太奇(射击) */
	UPROPERTY(EditAnywhere)
	UAnimMontage* ClientArmsFireAnimMontage;

	/* 持有动画蒙太奇(换弹) */
	UPROPERTY(EditAnywhere)
	UAnimMontage* ClientArmsReloadAnimMontage;

	/* 声效引用 */
	UPROPERTY(EditAnywhere)
	USoundBase* FireSound;

	/* 射击特效 */
	UPROPERTY(EditAnywhere)
	UParticleSystem* MuzzleFlash;

	/* 枪类型射击屏幕抖动 */
	UPROPERTY(EditAnywhere)
	TSubclassOf<UCameraShakeBase> CameraShakeClass;

	/* 手臂混合动画 */
	UPROPERTY(EditAnywhere)
	int FPArmsBlendPose;
	
protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	/* 枪体动画 这个方法在Cpp声明在蓝图实现 */
	UFUNCTION(BlueprintImplementableEvent,Category = "FPGunAnimation")
	void PlayShootAnimation();

	/* 枪体换弹动画 这个方法在Cpp声明在蓝图实现 */
	UFUNCTION(BlueprintImplementableEvent,Category = "FPGunAnimation")
	void PlayReloadAnimation();

	/* 枪体效果(粒子效果 音频声效) */
	void DisplayWeaponEffect();
};
