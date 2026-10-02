#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FunctionLibrary.generated.h"

class APlayerController;

UENUM(BlueprintType)
enum class EPrintLogLevel : uint8 {
  Info,
  Warning,
  Error
};

UENUM(BlueprintType)
enum class EViewportInputMode : uint8 {
  GameOnly,
  GameAndUI,
  UIOnly
};

/** Helper Functions */
UCLASS()
class APARTMENTS_API UGeneralFunctionLibrary : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  UFUNCTION(BlueprintCallable)
  static void PrintLog(const FString& Message, EPrintLogLevel Level = EPrintLogLevel::Info);

  UFUNCTION(BlueprintCallable)
  static void SetInputMode(APlayerController* PlayerController, EViewportInputMode Mode);
};
