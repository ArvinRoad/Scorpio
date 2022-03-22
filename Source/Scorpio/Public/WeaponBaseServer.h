#pragma once

#include "CoreMinimal.h"
#include "WeaponBaseClien.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Actor.h"
#include "WeaponBaseServer.generated.h"

UENUM()
enum class EWeaponType :uint8 {
	AK47		UMETA(DisplayName = "AK47"),	// 如果要中文必须改成 UTF-8 编码
	DesertEagle UMETA(DisplayName = "DesertEagle"),
	MP7			UMETA(DisplayName = "MP7"),
	Sniper		UMETA(DisplayName = "Sniper"),
	M4A1		UMETA(DisplayName = "M4A1")
};


UCLASS()
class SCORPIO_API AWeaponBaseServer : public AActor
{
	GENERATED_BODY()
	
public:	
	AWeaponBaseServer();

	UPROPERTY(EditAnywhere)
	EWeaponType KindOfWeapon;

	UPROPERTY(EditAnywhere)
	USkeletalMeshComponent* WeaponMesh;

	/* 碰撞体 */
	UPROPERTY(EditAnywhere)
	USphereComponent* SphereCollision;

	/* 动态创建第一人称客户端武器 */
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	TSubclassOf<AWeaponBaseClien> ClientWeaponBaseBPClass;

	UFUNCTION()
	void OnOtherBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);

	/* 卸载碰撞 物理 重力 */
	UFUNCTION()
	void EquipWeapon();
	
protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;
	
	/* 射击特效 */
	UPROPERTY(EditAnywhere)
	UParticleSystem* MuzzleFlash;

	/* 声效引用 */
	UPROPERTY(EditAnywhere)
	USoundBase* FireSound;
	
	// 枪体目前剩余子弹数
	UPROPERTY(EditAnywhere)
	int32 GunCurrentAmmo;
	// 弹匣目前剩余子弹数
	UPROPERTY(EditAnywhere,Replicated)
	int32 ClipCurrentAmmo;
	// 弹匣最多可以存多少子弹
	UPROPERTY(EditAnywhere)
	int32 MaxClipAmmo;

	UPROPERTY(EditAnywhere)
	UAnimMontage* ServerTPBodysShootAnimMontage;

	UPROPERTY(EditAnywhere)
	float BulletDistance;	// 子弹射击距离
	
	UPROPERTY(EditAnywhere)
	UMaterialInterface* BulletDecalMaterial;	// 弹孔贴图
	
	/* 多播 */
	UFUNCTION(NetMulticast,Reliable,WithValidation)
	void MultShootingEffect();
	void MultShootingEffect_Implementation();
	bool MultShootingEffect_Validate();
};
