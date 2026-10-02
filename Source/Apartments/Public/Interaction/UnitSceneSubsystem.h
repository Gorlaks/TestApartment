// Подсистема уровня: обновляет подсветку и вид квартир при выборе и фильтрации
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UnitSceneSubsystem.generated.h"

class UUnitComponent;

// Обновляет вид квартир, когда меняется выбор камеры или фильтр проданных
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
  // Слабые ссылки, чтобы не удерживать компоненты после удаления
  TArray<TWeakObjectPtr<UUnitComponent>> Units;
  FString SelectedUnitId;
  bool bHideSold = false;

  void RefreshUnits();
};
