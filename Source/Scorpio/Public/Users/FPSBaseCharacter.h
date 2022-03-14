// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
#include "FPSBaseCharacter.generated.h"

UCLASS()
class SCORPIO_API AFPSBaseCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
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
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
