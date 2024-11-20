/*
config.h - Estia R32 heat pump serial communication config
Copyright (C) 2025 serek4. All rights reserved.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, see <https://www.gnu.org/licenses/>.
*/

#ifndef CONFIG_H_
#define CONFIG_H_

#define SENSORS_DATA_TO_REQUEST "tc", "twi", "two", "tho", "wf", "lps", "te", "to", "td", "ts", "tl", "cmp", "fan1", "pmv", "hps"

#define TOSHIBA_ESTIA_MODEL 11    // 4kW, 6kW, 8kW, 11kW

#define MIN_HEATING_TEMP 20    // 20-37 (default 20)
#if TOSHIBA_ESTIA_MODEL < 8
#define MAX_HEATING_TEMP 55    // 37-55 (default 55) for 4kW and 6kW
#else
#define MAX_HEATING_TEMP 65    // 37-65 (default 65) for 8kW and 11kW
#endif
#define MIN_HOT_WATER_TEMP 40    // 40-60, (default 40)
#define MAX_HOT_WATER_TEMP 75    // 60-80, (default 75)

#endif
