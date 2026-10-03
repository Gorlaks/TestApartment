# Обновляет точки фокуса в json по обьектам на сцене
import json
import os
import unreal


def main():
  config_path = os.path.join(unreal.Paths.project_dir(), "Source", "Data", "data.json")
  with open(config_path, "r", encoding="utf-8") as file:
    config = json.load(file)

  apartments = {}
  for floor in config["building"]["floors"]:
    for apartment in floor["apartments"]:
      unit_id = str(apartment["id"]).strip()
      apartments[unit_id] = apartment

  positions = {}
  editor_actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
  for actor in editor_actors.get_all_level_actors():
    unit = actor.get_component_by_class(unreal.UnitComponent)
    if unit is None:
      continue

    unit_id = str(unit.get_editor_property("unit_id")).strip()
    if unit_id not in apartments:
      raise ValueError(f"ID {unit_id} у {actor.get_actor_label()} отсутствует в JSON")

    location, extent = actor.get_actor_bounds(False)
    positions[unit_id] = {
      "x": round(float(location.x), 2),
      "y": round(float(location.y), 2),
      "z": round(float(location.z), 2),
    }
    apartments[unit_id]["focus_point"] = positions[unit_id]

  for floor in config["building"]["floors"]:
    unit_ids = [str(apartment["id"]).strip() for apartment in floor["apartments"]]
    if all(unit_id in positions for unit_id in unit_ids):
      floor["focus_point"] = {
        axis: round(sum(positions[unit_id][axis] for unit_id in unit_ids) / len(unit_ids), 2)
        for axis in ("x", "y", "z")
      }

  if all(unit_id in positions for unit_id in apartments):
    config["building"]["genplan_focus_point"] = {
      axis: round(sum(position[axis] for position in positions.values()) / len(positions), 2)
      for axis in ("x", "y", "z")
    }

  temp_path = config_path + ".tmp"
  with open(temp_path, "w", encoding="utf-8", newline="\n") as file:
    json.dump(config, file, ensure_ascii=False, indent=2)
    file.write("\n")
  os.replace(temp_path, config_path)

  unreal.log(f"Обновлено квартир: {len(positions)}. Файл: {config_path}")

main()
