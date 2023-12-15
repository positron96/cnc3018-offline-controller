#include "MarlinDRO.h"
#include "gcode/gcode.h"

void MarlinDRO::begin() {
    DRO::begin();
    menuItems.push_back(MenuItem::simpleItem(2, "Home", [this](MenuItem &) {
        dev.scheduleCommand(G28_START_HOMING, 3);
    }));
    menuItems.push_back(MenuItem::simpleItem(3, "Set XY to 0", [this](MenuItem &) {
        dev.scheduleCommand("G92 X0Y0",8);
    }));
    menuItems.push_back(MenuItem::simpleItem(4, "Set Z to 0", [this](MenuItem &) {
        dev.scheduleCommand("G92 Z0",6);
    }));
    menuItems.push_back(MenuItem::simpleItem(5, "to Relative", [this](MenuItem &m) {
        dev.scheduleCommand(dev.isRelative() ? G90_SET_ABS_COORDINATES : G91_SET_RELATIVE_COORDINATES,3);
        dev.toggleRelative();
        m.text = dev.isRelative() ? "to Abs" : "to Relative";
    }));
    menuItems.push_back(MenuItem::simpleItem(6, "Goto XY=0", [this](MenuItem &) {
        dev.scheduleCommand("G0 X0Y0",7);
    }));
}
