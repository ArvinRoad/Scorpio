// Fill out your copyright notice in the Description page of Project Settings.


#include "KismetFPSLibrary.h"

void UKismetFPSLibrary::SortValues(TArray<FDeathMatchPlayerData>& Values) {
	Values.Sort( [](const FDeathMatchPlayerData& a,const FDeathMatchPlayerData& b)  {return a.PlayerScore > b.PlayerScore;} );
	//QuickSort(Values,0,Values.Num() - 1);
}

/* 手写快速排序法（与项目无关） */
TArray<FDeathMatchPlayerData>& UKismetFPSLibrary::QuickSort(TArray<FDeathMatchPlayerData>& Values, int start, int end) {
	if(start >= end) {
		return Values;
	}
	int i = start,j = end;
	FDeathMatchPlayerData Temp = Values[start];
	while (i != j) {
		while (j > i && Values[j].PlayerScore <= Temp.PlayerScore) {
			j--;
		}
		Values[i] = Values[j];
		while (j > i && Values[i].PlayerScore >= Temp.PlayerScore) {
			i++;
		}
		Values[j] = Values[i];
	}
	Values[i] = Temp;
	QuickSort(Values,start,i-1);
	QuickSort(Values,i+1,end);
	return Values;
}
