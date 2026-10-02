#include "FunctionLibrary.h"

#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogGeneral, Log, All);

void UGeneralFunctionLibrary::PrintLog(const FString& Message, EPrintLogLevel Level) {
  switch (Level) {
  case EPrintLogLevel::Warning:
    UE_LOG(LogGeneral, Warning, TEXT("%s"), *Message);
    break;

  case EPrintLogLevel::Error:
    UE_LOG(LogGeneral, Error, TEXT("%s"), *Message);
    break;

  case EPrintLogLevel::Info:
  default:
    UE_LOG(LogGeneral, Log, TEXT("%s"), *Message);
    break;
  }

  // Сообщение видно в игре и остаётся в Output Log
  if (GEngine) {
    FColor Color = FColor::White;
    if (Level == EPrintLogLevel::Warning) Color = FColor::Yellow;
    if (Level == EPrintLogLevel::Error) Color = FColor::Red;
    GEngine->AddOnScreenDebugMessage(-1, 5.0f, Color, Message);
  }
}

void UGeneralFunctionLibrary::SetInputMode(APlayerController* PlayerController, EViewportInputMode Mode) {
  if (!IsValid(PlayerController)) return;

  switch (Mode) {
  case EViewportInputMode::GameOnly:
    PlayerController->SetInputMode(FInputModeGameOnly());
    PlayerController->bShowMouseCursor = false;
    break;

  case EViewportInputMode::UIOnly:
    PlayerController->SetInputMode(FInputModeUIOnly());
    PlayerController->bShowMouseCursor = true;
    break;

  case EViewportInputMode::GameAndUI:
  default:
    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture(false);
    PlayerController->SetInputMode(InputMode);
    PlayerController->bShowMouseCursor = true;
    break;
  }
}
