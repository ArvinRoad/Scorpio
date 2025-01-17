// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "KismetFPSLibrary.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct FDeathMatchPlayerData {
	GENERATED_BODY()

	/* 排行榜 玩家名称数据 */
	UPROPERTY(BlueprintReadWrite)
	FName PlayerName;

	/* 排行榜 玩家分数数据 */
	UPROPERTY(BlueprintReadWrite)
	int PlayerScore;

	/* FDeathMatchPlayerData 构造函数 */
	FDeathMatchPlayerData() {
		PlayerName = "";
		PlayerScore = 0;
	}
};

UCLASS()
class SCORPIO_API UKismetFPSLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	/* 排行榜实时排序方法 */
	UFUNCTION(BlueprintCallable,Category="Sort")
	static void SortValues(UPARAM(ref)TArray<FDeathMatchPlayerData>& Values);

	/* 手写快速排序法（与项目无关） */
	static TArray<FDeathMatchPlayerData>& QuickSort(UPARAM(ref)TArray<FDeathMatchPlayerData>& Values,int start,int end);
	
};
