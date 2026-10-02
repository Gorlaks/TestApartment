#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UnitSceneSubsystem.generated.h"

class UUnitComponent;

// Keeps scene visuals in sync with camera selection and the sold filter.
UCLASS()
class APARTMENTS_API UUnitSceneSubsystem : public UWorldSubsystem {
  GENERATED_BODY()

public:
  void RegisterUnit(UUnitComponent* Unit);
  void UnregisterUnit(UUnitComponent* Unit);
  void RefreshUnit(UUnitComponent* Unit) const;
  void SetSelectedUnit(const FString& UnitId);
  void SetHideSold(bool bInHideSold);

private:
  TArray<TWeakObjectPtr<UUnitComponent>> Units;
  FString SelectedUnitId;
  bool bHideSold = false;

  void RefreshUnits();
};
