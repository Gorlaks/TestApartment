// Json парсер 
#pragma once

#include "DataTypes.h"

struct FConfigParseResult {
  bool bSuccess = false;
  FBuildingData Building;
  FString Message;
};

class FJsonParser {
public:
  static FConfigParseResult Parse(const FString& JsonText);
};
