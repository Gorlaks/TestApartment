#include "Interaction/UnitSceneSubsystem.h"

#include "Interaction/UnitComponent.h"

void UUnitSceneSubsystem::RegisterUnit(UUnitComponent* Unit) {
  if (!Unit) return;
  Units.AddUnique(TWeakObjectPtr<UUnitComponent>(Unit));
  RefreshUnit(Unit);
}

void UUnitSceneSubsystem::UnregisterUnit(UUnitComponent* Unit) {
  Units.Remove(TWeakObjectPtr<UUnitComponent>(Unit));
}

void UUnitSceneSubsystem::RefreshUnit(UUnitComponent* Unit) const {
  if (Unit) Unit->ApplyVisuals(!SelectedUnitId.IsEmpty() && Unit->GetUnitId() == SelectedUnitId, bHideSold);
}

void UUnitSceneSubsystem::SetSelectedUnit(const FString& UnitId) {
  if (SelectedUnitId == UnitId) return;
  SelectedUnitId = UnitId;
  RefreshUnits();
}

void UUnitSceneSubsystem::SetHideSold(bool bInHideSold) {
  if (bHideSold == bInHideSold) return;
  bHideSold = bInHideSold;
  RefreshUnits();
}

void UUnitSceneSubsystem::RefreshUnits() {
  for (int32 Index = Units.Num() - 1; Index >= 0; --Index) {
    UUnitComponent* Unit = Units[Index].Get();
    if (Unit) {
      RefreshUnit(Unit);
    } else {
      Units.RemoveAtSwap(Index);
    }
  }
}
