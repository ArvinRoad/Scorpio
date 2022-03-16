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
	
protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	/* 枪体动画 这个方法在Cpp声明在蓝图实现 */
	UFUNCTION(BlueprintImplementableEvent,Category = "FPGunAnimation")
	void PlayShootAnimation();
};
