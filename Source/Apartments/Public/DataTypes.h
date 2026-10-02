// Building data: apartments, floors and their focus points.
#pragma once

#include "CoreMinimal.h"
#include "DataTypes.generated.h"

UENUM(BlueprintType)
enum class EUnitStatus : uint8 {
  Free,
  Sold
};

USTRUCT(BlueprintType)
struct FUnitData {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString Id;

  UPROPERTY(BlueprintReadOnly)
  EUnitStatus Status = EUnitStatus::Free;

  UPROPERTY(BlueprintReadOnly)
  double AreaSqm = 0.0;

  UPROPERTY(BlueprintReadOnly)
  FVector FocusPoint = FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct FFloorData {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString Id;

  UPROPERTY(BlueprintReadOnly)
  int32 Number = 0;

  UPROPERTY(BlueprintReadOnly)
  FVector FocusPoint = FVector::ZeroVector;

  UPROPERTY(BlueprintReadOnly)
  TArray<FUnitData> Apartments;
};

USTRUCT(BlueprintType)
struct FBuildingData {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly)
  FString Id;

  UPROPERTY(BlueprintReadOnly)
  FString Name;

  UPROPERTY(BlueprintReadOnly)
  FVector GenplanFocusPoint = FVector::ZeroVector;

  UPROPERTY(BlueprintReadOnly)
  TArray<FFloorData> Floors;
};
