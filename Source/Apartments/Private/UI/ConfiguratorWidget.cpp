#include "UI/ConfiguratorWidget.h"

#include "DataSubsystem.h"
#include "General/FunctionLibrary.h"
#include "Interaction/UnitSceneSubsystem.h"
#include "UI/SelectionEntryWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

void UConfiguratorWidget::SetCameraPawn(ACameraPawn* InCameraPawn) {
  if (CameraPawn) {
    CameraPawn->OnViewChanged.RemoveDynamic(this, &UConfiguratorWidget::HandleViewChanged);
  }

  CameraPawn = InCameraPawn;
  if (GenplanButton) GenplanButton->SetIsEnabled(CameraPawn && DataSubsystem && DataSubsystem->HasData());
  if (CameraPawn) {
    CameraPawn->OnViewChanged.AddUniqueDynamic(this, &UConfiguratorWidget::HandleViewChanged);
    HandleViewChanged(CameraPawn->GetViewMode(), CameraPawn->GetSelectedId());
  } else {
    BackButton->SetIsEnabled(false);
    HideUnitCard();
  }
}

void UConfiguratorWidget::NativeOnInitialized() {
  Super::NativeOnInitialized();
  HideSoldCheckBox->OnCheckStateChanged.AddDynamic(this, &UConfiguratorWidget::HandleHideSoldChanged);
  BackButton->OnClicked.AddDynamic(this, &UConfiguratorWidget::HandleBackClicked);
  if (GenplanButton) GenplanButton->OnClicked.AddDynamic(this, &UConfiguratorWidget::HandleGenplanClicked);
  ReserveButton->OnClicked.AddDynamic(this, &UConfiguratorWidget::HandleReserveClicked);
}

void UConfiguratorWidget::NativeConstruct() {
  Super::NativeConstruct();

  if (UGameInstance* GameInstance = GetGameInstance()) {
    DataSubsystem = GameInstance->GetSubsystem<UDataSubsystem>();
  }
  if (DataSubsystem) {
    DataSubsystem->OnLoadFinished.AddUniqueDynamic(this, &UConfiguratorWidget::HandleDataLoaded);
  }

  bHideSold = HideSoldCheckBox->IsChecked();
  if (UUnitSceneSubsystem* Scene = GetWorld()->GetSubsystem<UUnitSceneSubsystem>()) {
    Scene->SetHideSold(bHideSold);
  }
  BackButton->SetIsEnabled(CameraPawn && CameraPawn->CanGoBack());
  if (GenplanButton) GenplanButton->SetIsEnabled(CameraPawn && DataSubsystem && DataSubsystem->HasData());
  ReserveButton->SetIsEnabled(false);
  HideUnitCard();
  // Виджет создаётся отдельно и JSON мог загрузиться ещё до открытия этого виджета
  RefreshFromData();
}

void UConfiguratorWidget::NativeDestruct() {
  if (DataSubsystem) DataSubsystem->OnLoadFinished.RemoveDynamic(this, &UConfiguratorWidget::HandleDataLoaded);
  if (CameraPawn) CameraPawn->OnViewChanged.RemoveDynamic(this, &UConfiguratorWidget::HandleViewChanged);
  DataSubsystem = nullptr;
  CameraPawn = nullptr;
  Super::NativeDestruct();
}

void UConfiguratorWidget::RefreshFromData() {
  if (!DataSubsystem || !DataSubsystem->HasData()) return;

  BuildingData = DataSubsystem->GetBuildingData();
  RebuildFloorList();
  DisplayedFloorId.Empty();
  UnitList->ClearChildren();
  UnitEntries.Empty();

  if (CameraPawn) HandleViewChanged(CameraPawn->GetViewMode(), CameraPawn->GetSelectedId());
}

void UConfiguratorWidget::RebuildFloorList() {
  FloorList->ClearChildren();
  if (!FloorEntryClass) return;

  // Верхний этаж показываем первым
  TArray<FFloorData> Floors = BuildingData.Floors;
  Floors.Sort([](const FFloorData& Left, const FFloorData& Right) {
    return Left.Number > Right.Number;
  });

  for (const FFloorData& Floor : Floors) {
    USelectionEntryWidget* Entry = CreateWidget<USelectionEntryWidget>(GetOwningPlayer(), FloorEntryClass);
    if (!Entry) continue;

    Entry->Setup(Floor.Id, FText::AsNumber(Floor.Number));
    Entry->OnSelected.AddDynamic(this, &UConfiguratorWidget::HandleFloorSelected);
    FloorList->AddChildToVerticalBox(Entry);
  }
}

void UConfiguratorWidget::ShowFloorUnits(const FFloorData& Floor) {
  if (DisplayedFloorId == Floor.Id) return;

  DisplayedFloorId = Floor.Id;
  UnitList->ClearChildren();
  UnitEntries.Empty();
  if (!UnitEntryClass) return;

  for (const FUnitData& Unit : Floor.Apartments) {
    USelectionEntryWidget* Entry = CreateWidget<USelectionEntryWidget>(GetOwningPlayer(), UnitEntryClass);
    if (!Entry) continue;

    const FString Label = FString::Printf(TEXT("%s | %.1f м² | %s"), *Unit.Id, Unit.AreaSqm,
      Unit.Status == EUnitStatus::Sold ? TEXT("Продано") : TEXT("Свободно"));
    Entry->Setup(Unit.Id, FText::FromString(Label), Unit.Status == EUnitStatus::Sold);
    Entry->SetHideSold(bHideSold);
    Entry->OnSelected.AddDynamic(this, &UConfiguratorWidget::HandleUnitSelected);
    UnitList->AddChildToVerticalBox(Entry);
    UnitEntries.Add(Entry);
  }
}

void UConfiguratorWidget::ShowUnitCard(const FUnitData& Unit) {
  SelectedUnitId = Unit.Id;
  UnitIdText->SetText(FText::FromString(Unit.Id));
  UnitAreaText->SetText(FText::AsNumber(Unit.AreaSqm));
  UnitStatusText->SetText(FText::FromString(
    Unit.Status == EUnitStatus::Sold ? TEXT("Продано") : TEXT("Свободно")));
  ReserveButton->SetIsEnabled(Unit.Status == EUnitStatus::Free);
  UnitCard->SetVisibility(ESlateVisibility::Visible);
}

void UConfiguratorWidget::HideUnitCard() {
  SelectedUnitId.Empty();
  UnitCard->SetVisibility(ESlateVisibility::Collapsed);
}

void UConfiguratorWidget::HandleDataLoaded(bool bSuccess, FString Message) {
  if (GenplanButton) GenplanButton->SetIsEnabled(bSuccess && CameraPawn);
  if (bSuccess) RefreshFromData();
}

void UConfiguratorWidget::HandleViewChanged(ECameraViewMode Mode, FString SelectedId) {
  BackButton->SetIsEnabled(CameraPawn && CameraPawn->CanGoBack());
  HideUnitCard();

  if (Mode == ECameraViewMode::Genplan) {
    DisplayedFloorId.Empty();
    UnitList->ClearChildren();
    UnitEntries.Empty();
    return;
  }

  for (const FFloorData& Floor : BuildingData.Floors) {
    if (Mode == ECameraViewMode::Floor && Floor.Id == SelectedId) {
      ShowFloorUnits(Floor);
      return;
    }

    if (Mode == ECameraViewMode::Unit) {
      for (const FUnitData& Unit : Floor.Apartments) {
        if (Unit.Id != SelectedId) continue;
        ShowFloorUnits(Floor);
        ShowUnitCard(Unit);
        return;
      }
    }
  }
}

void UConfiguratorWidget::HandleFloorSelected(FString Id) {
  if (!CameraPawn) return;
  for (const FFloorData& Floor : BuildingData.Floors) {
    if (Floor.Id == Id) {
      CameraPawn->ShowFloor(Floor.Number);
      return;
    }
  }
}

void UConfiguratorWidget::HandleUnitSelected(FString Id) {
  if (CameraPawn) CameraPawn->ShowUnit(Id);
}

void UConfiguratorWidget::HandleHideSoldChanged(bool bIsChecked) {
  bHideSold = bIsChecked;
  // Фильтр сразу меняет кнопки и вид квартир в сцене
  for (USelectionEntryWidget* Entry : UnitEntries) {
    if (Entry) Entry->SetHideSold(bHideSold);
  }
  if (UUnitSceneSubsystem* Scene = GetWorld()->GetSubsystem<UUnitSceneSubsystem>()) {
    Scene->SetHideSold(bHideSold);
  }
  OnSoldFilterChanged.Broadcast(bHideSold);
}

void UConfiguratorWidget::HandleBackClicked() {
  if (CameraPawn) CameraPawn->GoBack();
}

void UConfiguratorWidget::HandleGenplanClicked() {
  if (CameraPawn) CameraPawn->ShowGenplan();
}

void UConfiguratorWidget::HandleReserveClicked() {
  if (SelectedUnitId.IsEmpty()) return;
  UGeneralFunctionLibrary::PrintLog(FString::Printf(TEXT("Бронирование квартиры %s"), *SelectedUnitId));
}
