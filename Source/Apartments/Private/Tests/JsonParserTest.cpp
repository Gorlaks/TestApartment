#if WITH_DEV_AUTOMATION_TESTS

#include "JsonParser.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
  FJsonParserTest,
  "Data.JsonParser",
  EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FJsonParserTest::RunTest(const FString& Parameters) {
  const FString Json = TEXT(R"json(
{
  "building": {
    "id": "B1",
    "name": "Test",
    "genplan_focus_point": { "x": 0, "y": 0, "z": 100 },
    "floors": [{
      "id": "F1",
      "number": 1,
      "focus_point": { "x": 0, "y": 0, "z": 100 },
      "apartments": [
        { "id": "101", "status": "free", "area_sqm": 42.5,
          "focus_point": { "x": 10, "y": 20, "z": 100 } },
        { "id": "102", "status": "sold", "area_sqm": 50,
          "focus_point": { "x": 30, "y": 20, "z": 100 } }
      ]
    }]
  }
}
)json");

  const FConfigParseResult Result = FJsonParser::Parse(Json);
  TestTrue(TEXT("Valid configuration loads"), Result.bSuccess);
  TestEqual(TEXT("One floor loaded"), Result.Building.Floors.Num(), 1);
  if (Result.Building.Floors.Num() == 1) {
    TestEqual(TEXT("Two apartments loaded"), Result.Building.Floors[0].Apartments.Num(), 2);
    if (Result.Building.Floors[0].Apartments.Num() == 2) {
      TestTrue(TEXT("free maps to Free"), Result.Building.Floors[0].Apartments[0].Status == EUnitStatus::Free);
      TestTrue(TEXT("sold maps to Sold"), Result.Building.Floors[0].Apartments[1].Status == EUnitStatus::Sold);
    }
  }

  const FString InvalidStatusJson = Json.Replace(TEXT("\"sold\""), TEXT("\"Unknown\""));
  const FConfigParseResult InvalidStatus = FJsonParser::Parse(InvalidStatusJson);
  TestFalse(TEXT("Unknown status fails the whole configuration"), InvalidStatus.bSuccess);

  const FConfigParseResult InvalidRoot = FJsonParser::Parse(TEXT("{\"building\":{}}"));
  TestFalse(TEXT("Missing building fields fail safely"), InvalidRoot.bSuccess);
  return true;
}

#endif
