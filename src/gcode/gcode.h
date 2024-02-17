#pragma once

#ifndef GCODE__
#define GCODE__

#define G28_START_HOMING                 "G28"
#define G90_SET_ABS_COORDINATES          "G90"
#define G91_SET_RELATIVE_COORDINATES     "G91"
#define M0_STOP_UNCONDITIONAL            "M0"

#define M104_SET_EXTRUDER_TEMP           "M104"
#define M105_GET_EXTRUDER_TEMP           "M105"
#define M108_CONTINUE                    "M108"
#define M109_SET_EXTRUDER_TEMP_WAIT      "M109"
#define M110_SET_LINE_NUMBER             "M110"
#define RESET_LINE_NUMBER                "M110 N0"
#define M115_GET_FIRMWARE_VER            "M115"
#define M114_GET_CURRENT_POS             "M114"

// need to enable M114_REALTIME in marlin ConfigAdvanced.h
#define M114_REALTIME_VERSION            "M114 R"

#define M154_AUTO_REPORT_POSITION        "M154"
#define M155_AUTO_REPORT_TEMP            "M155"
#endif // GCODE__