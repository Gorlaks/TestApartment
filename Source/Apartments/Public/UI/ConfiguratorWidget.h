// Логика интерфейса конфигуратора здания
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "General/CameraPawn.h"
#include "DataTypes.h"
#include "ConfiguratorWidget.generated.h"

class UButton;
class UCheckBox;
class UDataSubsystem;
class USelectionEntryWidget;
class UTextBlock;
class UVerticalBox;
class UWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSoldFilterChanged, bool, bHideSold);

// Связывает данные, камеру и кнопки, но нешний вид настраивается уже в редакторе
UCLASS()
class APARTMENTS_API UConfiguratorWidget : public UUserWidget {
  GENERATED_BODY()

public:
  void SetCameraPawn(ACameraPawn* InCameraPawn);

  UFUNCTION(BlueprintPure)
  bool IsHidingSold() const { return bHideSold; }

  // Подпись на событие смены фильтра
  UPROPERTY(BlueprintAssignable)
  FOnSoldFilterChanged OnSoldFilterChanged;

protected:
  virtual void NativeOnInitialized() override;
  virtual void NativeConstruct() override;
  virtual void NativeDestruct() override;

private:
  UPROPERTY(EditDefaultsOnly)
  TSubclassOf<USelectionEntryWidget> FloorEntryClass;

  UPROPERTY(EditDefaultsOnly)
  TSubclassOf<USelectionEntryWidget> UnitEntryClass;

  // Поля необходимые для Blueprint виджета (создаются вручную в редакторе)
  UPROPERTY(meta = (BindWidget))
  TObjectPtr<UVerticalBox> FloorList;

  UPROPERTY(meta = (BindWidget))
  TObjectPtr<UVerticalBox> UnitList;

  UPROPERTY(meta = (BindWidget))
  TObjectPtr<UCheckBox> HideSoldCheckBox;

  UPROPERTY(meta = (BindWidget))
  TObjectPtr<UButton> BackButton;

  UPROPERTY(meta = (BindWidgetOptional))
  TObjectPtr<UButton> GenplanButton;

  UPROPERTY(meta = (BindWidget))
  TObjectPtr<UWidget> UnitCard;

  UPROPERTY(meta = (BindWidget))
  TObjectPtr<UTextBlock> UnitIdText;

  UPROPERTY(meta = (BindWidget))
  TObjectPtr<UTextBlock> UnitAreaText;

  UPROPERTY(meta = (BindWidget))
  TObjectPtr<UTextBlock> UnitStatusText;

  UPROPERTY(meta = (BindWidget))
  TObjectPtr<UButton> ReserveButton;

  UPROPERTY(Transient)
  TObjectPtr<UDataSubsystem> DataSubsystem;

  UPROPERTY(Transient)
  TObjectPtr<ACameraPawn> CameraPawn;

  UPROPERTY(Transient)
  TArray<TObjectPtr<USelectionEntryWidget>> UnitEntries;

  FBuildingData BuildingData;
  FString DisplayedFloorId;
  FString SelectedUnitId;
  bool bHideSold = false;

  void RefreshFromData();
  void RebuildFloorList();
  void ShowFloorUnits(const FFloorData& Floor);
  void ShowUnitCard(const FUnitData& Unit);
  void HideUnitCard();

  UFUNCTION()
  void HandleDataLoaded(bool bSuccess, FString Message);

  UFUNCTION()
  void HandleViewChanged(ECameraViewMode Mode, FString SelectedId);

  UFUNCTION()
  void HandleFloorSelected(FString Id);

  UFUNCTION()
  void HandleUnitSelected(FString Id);

  UFUNCTION()
  void HandleHideSoldChanged(bool bIsChecked);

  UFUNCTION()
  void HandleBackClicked();

  UFUNCTION()
  void HandleGenplanClicked();

  UFUNCTION()
  void HandleReserveClicked();
};
