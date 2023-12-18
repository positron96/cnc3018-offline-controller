#include "GrblDRO.h"
#include "FileChooser.h"

void GrblDRO::begin() {
    DRO::begin();
    menuItems.push_back(
            MenuItem::simpleItem(2, "Reset (Ctrl-X)", [this](MenuItem &) { dev.reset(); }));
    // menuItems.push_back( MenuItem::simpleItem(3, "Update", [this](MenuItem& m){
    //     enableRefresh(!isRefreshEnabled() );
    //     m.text = this->isRefreshEnabled() ? "Don't update" : "Update";
    //     setDirty(true);
    // }) );

    menuItems.push_back(MenuItem::simpleItem(3, "Home ($H)", [this](MenuItem &) {
        dev.schedulePriorityCommand("$H");
    }));
    menuItems.push_back(MenuItem::simpleItem(4, "Unlock ($X)", [this](MenuItem &) {
        dev.schedulePriorityCommand("$X");
    }));
    menuItems.push_back(MenuItem::simpleItem(5, "Set XYZ to 0", [this](MenuItem &) {
        // <G10 L2 P.. X.. Y.. Z..> L2 tells the G10 we’re setting standard work offsets
        //      P0 = Active coordinate system
        //      P1..6  = G54..59
        //      If it’s G90 (absolute)
        //      "G10 L2 P1 X10 Y20 Z0" Will set G54 to X10, Y20, and Z0.
        //        G91 (relative), then XYZ offset the current work offset’s
        //  <G10 L20 P.. X.. Y.. Z..> P can be used to select G54.1 P1..G54.1 P48
        dev.scheduleCommand("G10 L20 P1 X0Y0Z0");
    }));
    menuItems.push_back(MenuItem::simpleItem(6, "Goto XY=0", [this](MenuItem &) {
        dev.scheduleCommand("G0 X0Y0");
    }));
    menuItems.push_back(MenuItem::simpleItem(7, "Machine/Work", [this](MenuItem &) {
        useWCS = !useWCS;
        //dev.scheduleCommand(useWCS ? "G54" : "G53"); todo check this
    }));
};

void GrblDRO::drawAxisCoords(int sx, int sy, u_int8_t lineHeight) {
    float t[3] = {0, 0, 0};

    char ax[3];
    if (useWCS) {
        memcpy(ax, AXIS_WCS, 3);
        t[0] = dev.getXOfs();
        t[1] = dev.getYOfs();
        t[2] = dev.getZOfs();
    } else {
        memcpy(ax, AXIS, 3);
    }
    drawAxis(ax[0], dev.getX() - t[0], sx, sy);
    drawAxis(ax[1], dev.getY() - t[1], sx, sy + 11);
    drawAxis(ax[2], dev.getZ() - t[2], sx, sy + 11 * 2);
}