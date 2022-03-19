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
	UAnimInstance* ClientArmsEnemyBP;

	/* 屏幕抖动 */
	UPROPERTY(BlueprintReadOnly,meta=(AllowPrivateAccess="true"))
	AFPSPlayerController* FPSPlayerController;
	
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
	void LowSpeedWalkAction();		// 低速
	void NormalSpeedWalkAction();	// 正常速度
#pragma endregion
	
/* 武器相关 */
#pragma region Weapon
public:
	void EquipPrimary(AWeaponBaseServer* WeaponBaseServer);	// 主武器
private:
	UPROPERTY(EditAnywhere,meta=(AllowPrivateAccess = "true"))		// 后面去掉UPROPETY 当前没初始化
	EWeaponType ActiveWeapon;	// 当前使用武器类型
	
	UPROPERTY(meta=(AllowPrivateAccess = "true"))
	AWeaponBaseServer* ServerPrimaryWeapon;	// 服务器主武器的指针

	/* 接收ServerPrimaryWeapon  */
	UPROPERTY(meta=(AllowPrivateAccess = "true"))
	AWeaponBaseClien* ClientPrimaryWeapon;

	/* 使用某个枪开始 */
	void StartWithKindOfWeapon();
	void PurchaseWeapon(EWeaponType WeaponType);

	/* 现在使用客户端武器的指针 */
	AWeaponBaseClien* GetCurrentClientFPArmsWeaponAction();
	
#pragma endregion
	
private:
	virtual void Tick(float DeltaTime) override;
	
	/* 键盘绑定事件 */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

/* 换弹与射击相关 */
#pragma region Fire
public:
	void FireWeaponPrimary();	// 步枪射击方法
	void StopFirePrimary();	// 步枪停止射击回复事件
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

	/* 射击方法 */
	UFUNCTION(Server,Reliable,WithValidation)
	void ServerFireRifleWeapon(FVector CameraLocation,FRotator CameraRotation,bool IsMoving);
	void ServerFireRifleWeapon_Implementation(FVector CameraLocation,FRotator CameraRotation,bool IsMoving);
	bool ServerFireRifleWeapon_Validate(FVector CameraLocation,FRotator CameraRotation,bool IsMoving);

	/* 动态创建第一人称客户端武器 服务器下发客户端 服务器不需要生成 */
	UFUNCTION(Client,Reliable)
	void ClientEquipFPArmsPrimary();

	/* 枪体动画 */
	UFUNCTION(Client,Reliable)
	void ClientFire();

	/* 弹药更新 */
	UFUNCTION(Client,Reliable)
	void ClientUpdateAmmoUI(int32 ClipCurrentAmmo,int32 GunCurrentAmmo);
	
#pragma endregion 
};
