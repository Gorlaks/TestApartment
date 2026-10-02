#include "UI/SelectionEntryWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

void USelectionEntryWidget::NativeOnInitialized() {
  Super::NativeOnInitialized();
  SelectButton->OnClicked.AddDynamic(this, &USelectionEntryWidget::HandleClicked);
}

void USelectionEntryWidget::Setup(const FString& InId, const FText& InLabel, bool bInSold) {
  Id = InId;
  bSold = bInSold;
  Label->SetText(InLabel);
}

void USelectionEntryWidget::SetHideSold(bool bHideSold) {
  SelectButton->SetIsEnabled(!bSold || !bHideSold);
}

void USelectionEntryWidget::HandleClicked() {
  OnSelected.Broadcast(Id);
}
