import json
import os

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

def generate_header():
    reports_dir = '/home/hamed/projectes/papermc++/mc/generated/reports'
    
    header_content = [
        "// AUTOGENERATED FILE - DO NOT EDIT",
        "#ifndef PAPERMC_CORE_PROTOCOL_GENERATED_REGISTRIES_HPP",
        "#define PAPERMC_CORE_PROTOCOL_GENERATED_REGISTRIES_HPP",
        "",
        "#include <string>",
        "#include <vector>",
        "",
        "namespace papermc::core::protocol::generated {",
        ""
    ]

    all_registries = {}
    
    # Try parsing registries.json
    reg_path = os.path.join(reports_dir, 'registries.json')
    if os.path.exists(reg_path):
        with open(reg_path, 'r') as f:
            all_registries.update(json.load(f))
            
    # Try parsing datapack.json
    dp_path = os.path.join(reports_dir, 'datapack.json')
    if os.path.exists(dp_path):
        with open(dp_path, 'r') as f:
            dp = json.load(f)
            if 'registries' in dp:
                for k, v in dp['registries'].items():
                    if k not in all_registries:
                        all_registries[k] = v

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
        
        header_content.append(f"    inline const std::vector<std::string> {var_name} = {{")
        for i, key in enumerate(entry_keys):
            header_content.append(f"        \"{key}\"{',' if i < len(entry_keys)-1 else ''}")
        header_content.append("    };")
        header_content.append("")
            
    header_content.append("}")
    header_content.append("#endif // PAPERMC_CORE_PROTOCOL_GENERATED_REGISTRIES_HPP")

    os.makedirs('/home/hamed/projectes/papermc++/include/core/protocol', exist_ok=True)
    with open('/home/hamed/projectes/papermc++/include/core/protocol/generated_registries.hpp', 'w') as f:
        f.write("\n".join(header_content) + "\n")

if __name__ == '__main__':
    generate_header()
