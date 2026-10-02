#pragma once

#include "DataTypes.h"

struct FConfigParseResult {
  bool bSuccess = false;
  FBuildingData Building;
  FString Message;
};

/** Pure JSON parsing and validation; does not access UObjects or the world. */
class FJsonParser {
public:
  static FConfigParseResult Parse(const FString& JsonText);
};
