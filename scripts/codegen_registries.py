import json
import os
import glob
from collections import defaultdict

REGISTRIES_TO_EXTRACT = [
    "minecraft:damage_type",
    "minecraft:dimension_type",
    "minecraft:biome",
    "minecraft:cat_variant",
    "minecraft:chicken_variant",
    "minecraft:cow_variant",
    "minecraft:frog_variant",
    "minecraft:painting_variant",
    "minecraft:pig_variant",
    "minecraft:wolf_variant",
    "minecraft:zombie_nautilus_variant",
    "minecraft:cat_sound_variant",
    "minecraft:chicken_sound_variant",
    "minecraft:cow_sound_variant",
    "minecraft:pig_sound_variant",
    "minecraft:wolf_sound_variant",
    "minecraft:trim_material",
    "minecraft:trim_pattern",
    "minecraft:jukebox_song",
    "minecraft:banner_pattern"
]

FALLBACK_ENTRIES = {
    "minecraft:dimension_type": ["minecraft:overworld"],
    "minecraft:biome": ["minecraft:plains"],
    "minecraft:cat_variant": ["minecraft:all_black", "minecraft:black", "minecraft:british_shorthair", "minecraft:calico", "minecraft:jellie", "minecraft:persian", "minecraft:ragdoll", "minecraft:red", "minecraft:siamese", "minecraft:tabby", "minecraft:white"],
    "minecraft:chicken_variant": ["minecraft:cold", "minecraft:temperate", "minecraft:warm"],
    "minecraft:cow_variant": ["minecraft:cold", "minecraft:temperate", "minecraft:warm"],
    "minecraft:frog_variant": ["minecraft:cold", "minecraft:temperate", "minecraft:warm"],
    "minecraft:painting_variant": ["minecraft:kebab"],
    "minecraft:pig_variant": ["minecraft:cold", "minecraft:temperate", "minecraft:warm"],
    "minecraft:wolf_variant": ["minecraft:ashen", "minecraft:black", "minecraft:chestnut", "minecraft:pale", "minecraft:rusty", "minecraft:snowy", "minecraft:spotted", "minecraft:striped", "minecraft:woods"],
    "minecraft:zombie_nautilus_variant": ["minecraft:temperate"],
    "minecraft:cat_sound_variant": ["minecraft:classic"],
    "minecraft:chicken_sound_variant": ["minecraft:classic"],
    "minecraft:cow_sound_variant": ["minecraft:classic"],
    "minecraft:pig_sound_variant": ["minecraft:classic"],
    "minecraft:wolf_sound_variant": ["minecraft:classic"],
    "minecraft:trim_material": ["minecraft:quartz", "minecraft:iron", "minecraft:netherite", "minecraft:redstone", "minecraft:copper", "minecraft:gold", "minecraft:emerald", "minecraft:diamond", "minecraft:lapis", "minecraft:amethyst", "minecraft:resin"],
    "minecraft:trim_pattern": ["minecraft:sentry", "minecraft:dune", "minecraft:coast", "minecraft:wild", "minecraft:ward", "minecraft:eye", "minecraft:vex", "minecraft:tide", "minecraft:snout", "minecraft:rib", "minecraft:spire", "minecraft:wayfinder", "minecraft:raiser", "minecraft:shaper", "minecraft:host", "minecraft:silence", "minecraft:flow", "minecraft:bolt"],
    "minecraft:damage_type": ["minecraft:arrow", "minecraft:bad_respawn_point", "minecraft:cactus", "minecraft:campfire", "minecraft:cramming", "minecraft:dragon_breath", "minecraft:drown", "minecraft:dry_out", "minecraft:ender_pearl", "minecraft:explosion", "minecraft:fall", "minecraft:falling_anvil", "minecraft:falling_block", "minecraft:falling_stalactite", "minecraft:fireball", "minecraft:fireworks", "minecraft:fly_into_wall", "minecraft:freeze", "minecraft:generic", "minecraft:generic_kill", "minecraft:hot_floor", "minecraft:in_fire", "minecraft:in_wall", "minecraft:indirect_magic", "minecraft:lava", "minecraft:lightning_bolt", "minecraft:mace_smash", "minecraft:magic", "minecraft:mob_attack", "minecraft:mob_attack_no_aggro", "minecraft:on_fire", "minecraft:out_of_world", "minecraft:outside_border", "minecraft:player_attack", "minecraft:player_explosion", "minecraft:sonic_boom", "minecraft:spear", "minecraft:spit", "minecraft:stalagmite", "minecraft:starve", "minecraft:sting", "minecraft:sweet_berry_bush", "minecraft:thorns", "minecraft:thrown", "minecraft:trident", "minecraft:wind_charge", "minecraft:wither", "minecraft:wither_skull"],
    "minecraft:jukebox_song": ["minecraft:11", "minecraft:13", "minecraft:5", "minecraft:blocks", "minecraft:bounce", "minecraft:cat", "minecraft:chirp", "minecraft:creator", "minecraft:creator_music_box", "minecraft:far", "minecraft:lava_chicken", "minecraft:mall", "minecraft:mellohi", "minecraft:otherside", "minecraft:pigstep", "minecraft:precipice", "minecraft:relic", "minecraft:stal", "minecraft:strad", "minecraft:tears", "minecraft:wait", "minecraft:ward"],
    "minecraft:banner_pattern": ["minecraft:base", "minecraft:square_bottom_left", "minecraft:square_bottom_right", "minecraft:square_top_left", "minecraft:square_top_right", "minecraft:stripe_bottom", "minecraft:stripe_top", "minecraft:stripe_left", "minecraft:stripe_right", "minecraft:stripe_center", "minecraft:stripe_middle", "minecraft:stripe_downright", "minecraft:stripe_downleft", "minecraft:small_stripes", "minecraft:cross", "minecraft:straight_cross", "minecraft:triangle_bottom", "minecraft:triangle_top", "minecraft:triangles_bottom", "minecraft:triangles_top", "minecraft:diagonal_left", "minecraft:diagonal_right", "minecraft:diagonal_up_left", "minecraft:diagonal_up_right", "minecraft:circle", "minecraft:rhombus", "minecraft:half_vertical", "minecraft:half_horizontal", "minecraft:half_vertical_right", "minecraft:half_horizontal_bottom", "minecraft:border", "minecraft:curly_border", "minecraft:gradient", "minecraft:gradient_up", "minecraft:bricks", "minecraft:globe", "minecraft:creeper", "minecraft:skull", "minecraft:flower", "minecraft:mojang", "minecraft:piglin", "minecraft:flow", "minecraft:guster"]
}

def load_json(path):
    with open(path, 'r') as f:
        return json.load(f)

def flatten_tags(tag_dict, tag_name, resolved, resolving):
    if tag_name in resolved:
        return resolved[tag_name]
    
    if tag_name in resolving:
        raise Exception(f"Circular tag reference detected involving {tag_name}")
    
    resolving.add(tag_name)
    result = []
    
    if tag_name in tag_dict:
        for val in tag_dict[tag_name]:
            if val.startswith('#'):
                result.extend(flatten_tags(tag_dict, val[1:], resolved, resolving))
            else:
                result.append(val)
                
    resolved[tag_name] = result
    resolving.remove(tag_name)
    return result

def generate_header():
    reports_dir = '/home/hamed/projectes/papermc++/mc/generated/reports'
    tags_dir = '/home/hamed/projectes/papermc++/mc/generated/data/minecraft/tags'
    
    header_content = [
        "// AUTOGENERATED FILE - DO NOT EDIT",
        "#ifndef PAPERMC_CORE_PROTOCOL_GENERATED_REGISTRIES_HPP",
        "#define PAPERMC_CORE_PROTOCOL_GENERATED_REGISTRIES_HPP",
        "",
        "#include <string>",
        "#include <vector>",
        "#include <cstdint>",
        "",
        "namespace papermc::core::protocol::generated {",
        ""
    ]

    all_registries = {}
    
    reg_path = os.path.join(reports_dir, 'registries.json')
    if os.path.exists(reg_path):
        all_registries.update(load_json(reg_path))
            
    dp_path = os.path.join(reports_dir, 'datapack.json')
    if os.path.exists(dp_path):
        dp = load_json(dp_path)
        if 'registries' in dp:
            for k, v in dp['registries'].items():
                if k not in all_registries:
                    all_registries[k] = v

    registry_id_map = {} # registry_name -> {entry_name -> int_id}
    
    # 1. Build map for static registries from registries.json
    for reg_name, reg_data in all_registries.items():
        if "entries" in reg_data and isinstance(reg_data["entries"], dict):
            # some entries might not have protocol_id? they usually do if they are static
            reg_map = {}
            for entry_k, entry_v in reg_data["entries"].items():
                if "protocol_id" in entry_v:
                    reg_map[entry_k] = entry_v["protocol_id"]
            if reg_map:
                registry_id_map[reg_name] = reg_map

    # 2. Extract dynamic registries and build their maps
    for reg_name in REGISTRIES_TO_EXTRACT:
        entry_keys = []
        if reg_name in all_registries and "entries" in all_registries[reg_name]:
            entries = all_registries[reg_name]["entries"]
            if isinstance(entries, dict):
                entry_keys = list(entries.keys())
            elif isinstance(entries, list):
                entry_keys = entries
                
        if not entry_keys and reg_name in FALLBACK_ENTRIES:
            entry_keys = FALLBACK_ENTRIES[reg_name]
            
        var_name = reg_name.split(":")[-1] + "_entries"
        
        # Build map based on index for dynamic registries
        registry_id_map[reg_name] = {k: i for i, k in enumerate(entry_keys)}
        
        header_content.append(f"    inline const std::vector<std::string> {var_name} = {{")
        for i, key in enumerate(entry_keys):
            header_content.append(f"        \"{key}\"{',' if i < len(entry_keys)-1 else ''}")
        header_content.append("    };")
        header_content.append("")
        
    # 3. Parse all tags recursively
    all_tags = {} # registry_name -> { tag_name -> [entry_ids...] }
    
    for root, dirs, files in os.walk(tags_dir):
        for file in files:
            if not file.endswith('.json'):
                continue
                
            file_path = os.path.join(root, file)
            rel_path = os.path.relpath(file_path, tags_dir)
            parts = rel_path.split(os.sep)
            
            # e.g. parts = ['worldgen', 'biome', 'has_structure', 'village_desert.json']
            # category is 'worldgen/biome'
            # tag name is 'minecraft:has_structure/village_desert'
            
            # Find the category: we'll match it with registry names.
            # Usually the category matches registry name exactly, e.g. 'damage_type', 'banner_pattern', 'worldgen/biome'
            
            if len(parts) >= 2:
                # Try to guess category length by matching known registries
                matched_category = None
                for i in range(len(parts), 0, -1):
                    possible_cat = '/'.join(parts[:i])
                    if f"minecraft:{possible_cat}" in registry_id_map or possible_cat in ["block", "item", "entity_type", "fluid", "game_event"]:
                        matched_category = possible_cat
                        break
                
                if not matched_category:
                    matched_category = parts[0]
            else:
                matched_category = parts[0]
                
            reg_name = f"minecraft:{matched_category}"
            if reg_name not in all_tags:
                all_tags[reg_name] = {}
                
            tag_rel_path = '/'.join(parts[len(matched_category.split('/')):])
            tag_name = f"minecraft:{tag_rel_path[:-5]}" # remove .json
            
            tag_data = load_json(file_path)
            values = []
            for val in tag_data.get('values', []):
                if isinstance(val, dict):
                    # Sometimes it's an object {"id": "...", "required": false}
                    if val.get('required', True) == False:
                        continue
                    values.append(val['id'])
                else:
                    values.append(val)
                    
            all_tags[reg_name][tag_name] = values

    # 4. Flatten tags and map to IDs
    tag_structs = []
    
    for reg_name, tags in all_tags.items():
        if reg_name not in registry_id_map:
            # Skip if we don't have IDs for this registry
            continue
            
        resolved = {}
        resolving = set()
        
        reg_id_map = registry_id_map[reg_name]
        
        for tag_name in tags.keys():
            flatten_tags(tags, tag_name, resolved, resolving)
            
        # Write to C++ structs
        tag_str = f"        {{\"{reg_name}\", {{\n"
        tag_entries = []
        for tag_name, entries in resolved.items():
            numerical_ids = []
            for e in entries:
                if e in reg_id_map:
                    numerical_ids.append(reg_id_map[e])
                else:
                    e_mc = e if e.startswith('minecraft:') else f"minecraft:{e}"
                    if e_mc in reg_id_map:
                        numerical_ids.append(reg_id_map[e_mc])
                        
            # Deduplicate IDs
            numerical_ids = list(dict.fromkeys(numerical_ids))
            
            if numerical_ids:
                ids_str = ", ".join(map(str, numerical_ids))
                tag_entries.append(f"            {{\"{tag_name}\", {{{ids_str}}}}}")
            else:
                tag_entries.append(f"            {{\"{tag_name}\", {{}}}}")
                
        tag_str += ",\n".join(tag_entries)
        tag_str += "\n        }}"
        tag_structs.append(tag_str)

    header_content.append("    inline const std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::vector<int32_t>>>>> registry_tags = {")
    header_content.append(",\n".join(tag_structs))
    header_content.append("    };")
    header_content.append("")
    header_content.append("}")
    header_content.append("#endif // PAPERMC_CORE_PROTOCOL_GENERATED_REGISTRIES_HPP")

    os.makedirs('/home/hamed/projectes/papermc++/include/core/protocol', exist_ok=True)
    with open('/home/hamed/projectes/papermc++/include/core/protocol/generated_registries.hpp', 'w') as f:
        f.write("\n".join(header_content) + "\n")

if __name__ == '__main__':
    generate_header()
