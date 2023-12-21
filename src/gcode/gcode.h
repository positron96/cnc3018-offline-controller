#pragma once

#ifndef GCODE__
#define GCODE__

#define G28_START_HOMING                 "G28"
#define G90_SET_ABS_COORDINATES          "G90"
#define G91_SET_RELATIVE_COORDINATES     "G91"
#define M104_SET_EXTRUDER_TEMP           "M104"
#define M105_GET_EXTRUDER_TEMP           "M105"
#define M109_SET_EXTRUDER_TEMP_WAIT      "M109"
#define M115_GET_FIRMWARE_VER            "M115"
#define M114_GET_CURRENT_POS             "M114"

#define M154_AUTO_REPORT_POSITION        "M154"
#define M155_AUTO_REPORT_TEMP            "M155"
#endif // GCODE__