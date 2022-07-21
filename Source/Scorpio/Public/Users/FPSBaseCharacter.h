#pragma once

#include "CoreMinimal.h"
#include "FPSPlayerController.h"
#include "WeaponBaseServer.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
#include "FPSBaseCharacter.generated.h"

UCLASS()
class SCORPIO_API AFPSBaseCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AFPSBaseCharacter();

	/* BeginPlay 延迟 */
	UFUNCTION()
	void DelayBeginPlayCallBack();
	
/**	函数说明：
 *  PlayerCamera 摄像机组件
 *  FPArmsMesh   手臂
 */
#pragma region component
private:
	UPROPERTY(Category=Character,VisibleAnywhere,BlueprintReadOnly,meta=(AllowPrivateAccess = "true"))
	UCameraComponent* PlayerCamera;

	UPROPERTY(Category=Character,VisibleAnywhere,BlueprintReadOnly,meta=(AllowPrivateAccess="true"));
	USkeletalMeshComponent* FPArmsMesh;

	/* 动画蓝图 */
	UPROPERTY(Category=Character,BlueprintReadOnly,meta=(AllowPrivateAccess="true"));
	UAnimInstance* ClientArmsAnimBP;
	UPROPERTY(Category=Character,BlueprintReadOnly,meta=(AllowPrivateAccess="true"));
	UAnimInstance* ServerBodysAnimBP;

	/* 屏幕抖动 */
	UPROPERTY(BlueprintReadOnly,meta=(AllowPrivateAccess="true"))
	AFPSPlayerController* FPSPlayerController;

	/* 测试代码：更替开局初始武器 */
	UPROPERTY(EditAnywhere)
	EWeaponType TestStartWeapon;

#pragma endregion 

protected:
	virtual void BeginPlay() override;

/* 键盘输入事件 */
#pragma region InputEvent
	void MoveForward(float AxisValue);
	void MoveRight(float AxisValue);
	
	void JumpAction();
	void StopJumpAction();
	void InputFirePressed();	// 鼠标左键按下的回调
	void InputFireReleased();	// 鼠标左键松手松手
	void InputAimingPressed();	// 狙击枪开镜的回调
	void InputAimingReleased();	// 鼠标左键松手松手
	void LowSpeedWalkAction();		// 低速
	void NormalSpeedWalkAction();	// 正常速度
	void InputReload();	// 换弹方法
#pragma endregion
	
/* 武器相关 */
#pragma region Weapon
public:
	void EquipPrimary(AWeaponBaseServer* WeaponBaseServer);	// 主武器
	void EquipSecondary(AWeaponBaseServer* WeaponBaseServer);	// 副武器

	/* 手臂混合动画方法 */
	UFUNCTION(BlueprintImplementableEvent)
	void UpdateFPArmsBlendPose(int NewIndex);
	
private:
	UPROPERTY(meta=(AllowPrivateAccess = "true"),Replicated)		// 后面去掉UPROPETY 当前没初始化
	EWeaponType ActiveWeapon;	// 当前使用武器类型
	
	UPROPERTY(meta=(AllowPrivateAccess = "true"))
	AWeaponBaseServer* ServerPrimaryWeapon;	// 服务器主武器的指针

	/* 接收ServerPrimaryWeapon  */
	UPROPERTY(meta=(AllowPrivateAccess = "true"))
	AWeaponBaseClien* ClientPrimaryWeapon;

	UPROPERTY(meta=(AllowPrivateAccess = "true"))
	AWeaponBaseServer* ServerSecondaryWeapon;	//	服务器副武器的指针

	/* 接收ServerSecondaryWeapon */
	UPROPERTY(meta=(AllowPrivateAccess = "true"))
	AWeaponBaseClien* ClientSecondaryWeapon;

	/* 使用某个枪开始 */
	void StartWithKindOfWeapon();
	void PurchaseWeapon(EWeaponType WeaponType);

	/* 现在使用客户端武器的指针 */
	AWeaponBaseClien* GetCurrentClientFPArmsWeaponAction();

	/* 现在使用服务端武器指针 */
	AWeaponBaseServer* GetCurrentServerTPBodysWeaponAtcor();
	
#pragma endregion
	
private:
	virtual void Tick(float DeltaTime) override;
	
	/* 键盘绑定事件 */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

/* 换弹与射击相关 */
#pragma region Fire
public:
	/* 计时器 */
	FTimerHandle AutomaticFireTimerHandle;	// 自动射击计时器(连射)
	void AutomaticFire();	// 自动射击方法

	/* 后坐力参数 */
	float NewVerticalRecoilAmount;
	float OldVerticalRecoilAmount;
	float VerticalRecoilAmount;
	float RecoilXCoordPerShoot;
	void ResetRecoil();	// 重置后坐力相关变量
	float NewHorizontalRecoilAmount;
	float OldHorizontalRecoilAmount;
	float HorizontalRecoilAmount;

	/* 手枪射击后坐力偏移值变量 */
	float PistolSpreadMin = 0;
	float PistolSpreadMax = 0;
	
	/* 步枪相关射击方法 */
	void FireWeaponPrimary();	// 步枪射击方法
	void StopFirePrimary();	// 步枪停止射击回复事件
	void RifleLineTrace(FVector CameraLocation,FRotator CameraRotation,bool IsMoving);	// 步枪射线检测

	/* 狙击枪相关射击方法 */
	void FireWeaponSniper();	// 狙击枪射击方法
	void StopFireSniper();		// 狙击枪停止回复事件
	void SniperLineTrace(FVector CameraLocation,FRotator CameraRotation,bool IsMoving);	// 狙击枪射线检测

	/* 狙击枪是否开镜参数 */
	UPROPERTY(Replicated)
	bool IsAiming;

	/* 狙击枪瞄准的UI接收指针 */
	UPROPERTY(VisibleAnywhere,Category="SniperUI")
	UUserWidget* WidgetScope;

	/* 狙击枪瞄准UI 接收蓝图类指针 */
	UPROPERTY(EditAnywhere,Category="SniperUI")
	TSubclassOf<UUserWidget> SniperScopeBPClass;
	
	/* 手枪相关射击方法 */
	void FireWeaponSecondary();	// 手枪射击方法
	void StopFireSecondary();	// 手枪停止射击回复事件
	void PistolLineTrace(FVector CameraLocation,FRotator CameraRotation,bool IsMoving);	// 手枪射线检测
	
	/* Reload */
	UPROPERTY(Replicated)
	bool IsFiring;
	UPROPERTY(Replicated)
	bool IsReloading;
	
	UFUNCTION()
	void DelayPlayArmReloadCallBack();	// 步枪换弹动画后的回调 计时器回调

	UFUNCTION()
	void DelaySpreadWeaponShootCallBack();	// 手枪换弹动画后的回调 计时器回调

	UFUNCTION()
	void DelaySniperShootCallBack();	// 狙击枪换弹动画后的回调 计时器回调
	
	void DamagePlayer(UPhysicalMaterial* PhysicalMaterial,AActor* DamagedActor,FVector& HitFromDirection,FHitResult& HitInfo);	// 玩家伤害(五个部位)
	UFUNCTION()
	void OnHit(AActor* DamagedActor, float Damage, class AController* InstigatedBy, FVector HitLocation, class UPrimitiveComponent* FHitComponent, FName BoneName, FVector ShotFromDirection, const class UDamageType* DamageType, AActor* DamageCauser); // 伤害回调方法

	float Health;	// 角色生命变量
	
	void DeathMatchDeath(AActor* DamageActor);	// 死亡竞技死亡方法
#pragma endregion

	/* 服务器同步交互 */
public:
#pragma region NetWorking

	/* 静步服务器同步 */
	UFUNCTION(Server,Reliable,WithValidation)
	void ServerLowSpeedWalkAction();
	void ServerLowSpeedWalkAction_Implementation();
	bool ServerLowSpeedWalkAction_Validate();
	
	UFUNCTION(Server,Reliable,WithValidation)
	void ServerNormalSpeedWalkAction();
	void ServerNormalSpeedWalkAction_Implementation();
	bool ServerNormalSpeedWalkAction_Validate();

	/* 步枪射击方法 */
	UFUNCTION(Server,Reliable,WithValidation)
	void ServerFireRifleWeapon(FVector CameraLocation,FRotator CameraRotation,bool IsMoving);
	void ServerFireRifleWeapon_Implementation(FVector CameraLocation,FRotator CameraRotation,bool IsMoving);
	bool ServerFireRifleWeapon_Validate(FVector CameraLocation,FRotator CameraRotation,bool IsMoving);

	/* 狙击枪射击方法 */
	UFUNCTION(Server,Reliable,WithValidation)
	void ServerFireSniperWeapon(FVector CameraLocation,FRotator CameraRotation,bool IsMoving);
	void ServerFireSniperWeapon_Implementation(FVector CameraLocation,FRotator CameraRotation,bool IsMoving);
	bool ServerFireSniperWeapon_Validate(FVector CameraLocation,FRotator CameraRotation,bool IsMoving);
	
	/* 手枪射击方法 */
	UFUNCTION(Server,Reliable,WithValidation)
	void ServerFirePistolWeapon(FVector CameraLocation,FRotator CameraRotation,bool IsMoving);
	void ServerFirePistolWeapon_Implementation(FVector CameraLocation,FRotator CameraRotation,bool IsMoving);
	bool ServerFirePistolWeapon_Validate(FVector CameraLocation,FRotator CameraRotation,bool IsMoving);
	
	/* 步枪换弹方法 */
	UFUNCTION(Server,Reliable,WithValidation)
	void ServerReloadPrimary();
	void ServerReloadPrimary_Implementation();
	bool ServerReloadPrimary_Validate();

	/* 手枪换挡方法 */
	UFUNCTION(Server,Reliable,WithValidation)
	void ServerReloadSecondary();
	void ServerReloadSecondary_Implementation();
	bool ServerReloadSecondary_Validate();
	
	/* 射击停止状态方法 */
	UFUNCTION(Server,Reliable,WithValidation)
	void ServerStopFiring();
	void ServerStopFiring_Implementation();
	bool ServerStopFiring_Validate();

	/* 狙击枪开镜方法 */
	UFUNCTION(Server,Reliable,WithValidation)
	void ServerSetAiming(bool AimingState);
	void ServerSetAiming_Implementation(bool AimingState);
	bool ServerSetAiming_Validate(bool AimingState);

	/* 多播 身体射击蒙太奇动画 */
	UFUNCTION(NetMulticast,Reliable,WithValidation)
	void MultShooting();
	void MultShooting_Implementation();
	bool MultShooting_Validate();

	/* 多播 身体换弹蒙太奇动画 */
	UFUNCTION(NetMulticast,Reliable,WithValidation)
	void MultiReloadAnimation();
	void MultiReloadAnimation_Implementation();
	bool MultiReloadAnimation_Validate();

	/* 多播 生成弹孔 */
	UFUNCTION(NetMulticast,Reliable,WithValidation)
	void MultiSpawnBulletDecal(FVector Location,FRotator Rotation);
	void MultiSpawnBulletDecal_Implementation(FVector Location,FRotator Rotation);
	bool MultiSpawnBulletDecal_Validate(FVector Location,FRotator Rotation);

	/* 动态创建第一人称客户端主武器 服务器下发客户端 服务器不需要生成 */
	UFUNCTION(Client,Reliable)
	void ClientEquipFPArmsPrimary();

	/* 动态创建第一人称客户端副武器 服务器下发客户端 服务器不需要生成 */
	UFUNCTION(Client,Reliable)
	void ClientEquipFPArmsSecondary();

	/* 枪体动画 */
	UFUNCTION(Client,Reliable)
	void ClientFire();

	/* 弹药更新 */
	UFUNCTION(Client,Reliable)
	void ClientUpdateAmmoUI(int32 ClipCurrentAmmo,int32 GunCurrentAmmo);

	/* 玩家血量更新 */
	UFUNCTION(Client,Reliable)
	void ClientUpdateHealthUI(float NewHealth);

	/* 客户端后坐力方法 */
	UFUNCTION(Client,Reliable)
	void ClientRecoil();

	/* 客户端换弹动画方法 */
	UFUNCTION(Client,Reliable)
	void ClientReload();

	/* 狙击枪开镜客户端方法 */
	UFUNCTION(Client,Reliable)
	void ClientAiming();

	/* 狙击枪开镜回调客户端方法 */
	UFUNCTION(Client,Reliable)
	void ClientEndAiming();

	/**/
	UFUNCTION(Client,Reliable)
	void ClientDeathMathDeath();
	
#pragma endregion 
};
