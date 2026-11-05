// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string_view>

#include "common/common_types.h"

namespace QtCommon::Game {

/// Names for titles whose dumps often lack control data, such as loose ExeFS/RomFS dumps.
/// Empty when the title is not listed.
constexpr std::string_view KnownTitleName(u64 title_id) {
    switch (title_id) {
    case 0x0400C3F00006E000ULL:
        return "Mario Kart World";
    case 0x040074A01BF12000ULL:
        return "Donkey Kong Bananza";
    case 0x040077201BE50000ULL:
        return "Kirby Air Riders";
    case 0x04005B902517C000ULL:
        return "Kirby Air Riders Global Test Ride";
    case 0x04007C6028AA8000ULL:
        return "Yoshi and the Mysterious Book";
    case 0x0400D4201C6EA000ULL:
        return "Mario Tennis Fever";
    case 0x0400E88025B1A000ULL:
        return "Star Fox";
    case 0x04007EF01F8AE000ULL:
        return "Fire Emblem: Fortune's Weave";
    case 0x04008E001F098000ULL:
        return "Nintendo Switch Sports Resort";
    case 0x0400A3A01EE4C000ULL:
        return "Pokemon Pokopia";
    case 0x0400A4802437E000ULL:
        return "Drag x Drive";
    case 0x040041E0212AE000ULL:
        return "Fast Fusion";
    case 0x040057C02159A000ULL:
        return "Nintendo Switch 2 Welcome Tour";
    case 0x0400486024DEC000ULL:
        return "The Legend of Zelda: Ocarina of Time";
    case 0x0100000000010000ULL:
        return "Super Mario Odyssey";
    case 0x0100152000022000ULL:
        return "Mario Kart 8 Deluxe";
    case 0x01007EF00011E000ULL:
        return "The Legend of Zelda: Breath of the Wild";
    case 0x0100F2C0115B6000ULL:
        return "The Legend of Zelda: Tears of the Kingdom";
    case 0x01006A800016E000ULL:
        return "Super Smash Bros. Ultimate";
    case 0x01006F8002326000ULL:
        return "Animal Crossing: New Horizons";
    default:
        return {};
    }
}

} // namespace QtCommon::Game
