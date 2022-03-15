#pragma once

#include "CoreMinimal.h"
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
	
protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

};
