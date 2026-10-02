// Выбор этажа или квартиры в UI
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SelectionEntryWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEntrySelected, FString, Id);

// Внешний вид уже в самом редакторе
UCLASS()
class APARTMENTS_API USelectionEntryWidget : public UUserWidget {
  GENERATED_BODY()

public:
  void Setup(const FString& InId, const FText& InLabel, bool bInSold = false);
  void SetHideSold(bool bHideSold);

  UPROPERTY(BlueprintAssignable)
  FOnEntrySelected OnSelected;

protected:
  virtual void NativeOnInitialized() override;

private:
  UPROPERTY(meta = (BindWidget))
  TObjectPtr<UButton> SelectButton;

  UPROPERTY(meta = (BindWidget))
  TObjectPtr<UTextBlock> Label;

  FString Id;
  bool bSold = false;

  UFUNCTION()
  void HandleClicked();
};
