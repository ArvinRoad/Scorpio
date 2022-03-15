#pragma once

#include "CoreMinimal.h"
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
#pragma endregion 

protected:
	virtual void BeginPlay() override;

/* 键盘输入事件 */
#pragma region InputEvent
	void MoveForward(float AxisValue);
	void MoveRight(float AxisValue);
	
	void JumpAction();
	void StopJumpAction();
	void LowSpeedWalkAction();		// 低速
	void NormalSpeedWalkAction();	// 正常速度
#pragma endregion
	
/* 武器相关 */
#pragma region Weapon
public:
	void EquipPrimary(AWeaponBaseServer* WeaponBaseServer);	// 主武器
private:
	UPROPERTY(meta=(AllowPrivateAccess = "ture"))
	AWeaponBaseServer* ServerPrimaryWeapon;	// 服务器主武器的指针

#pragma endregion 
	
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

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

#pragma endregion 
};
