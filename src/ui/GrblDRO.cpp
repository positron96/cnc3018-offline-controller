#include "GrblDRO.h"

//#include "FileChooser.h"


//extern FileChooser fileChooser;

    void GrblDRO::begin() {
        DRO::begin();
        // menuItems.push_back( MenuItem::simpleItem(0, 'o', [](MenuItem&){  Display::getDisplay()->setScreen(&fileChooser); }) );
        // menuItems.push_back( MenuItem::simpleItem(0, 'p', [this](MenuItem& m){   
        //     Job *job = Job::getJob();
        //     if(!job->isRunning() ) return;
        //     job->setPaused(!job->isPaused());
        //     m.glyph = job->isPaused() ? 'r':'p';
        //     setDirty(true);
        // }) );
        // menuItems.push_back( MenuItem::simpleItem(1, 'x', [](MenuItem&){  GCodeDevice::getDevice()->reset(); }) );
        // menuItems.push_back( MenuItem::simpleItem(2, 'u', [this](MenuItem& m){  
        //     enableRefresh(!isRefreshEnabled() );
        //     m.glyph = this->isRefreshEnabled() ? 'u' : 'U';
        //     setDirty(true);
        // }) );

        // menuItems.push_back( MenuItem::simpleItem(3, 'H', [](MenuItem&){  GCodeDevice::getDevice()->schedulePriorityCommand("$H"); }) );
        // menuItems.push_back( MenuItem::simpleItem(4, 'w', [](MenuItem&){  GCodeDevice::getDevice()->scheduleCommand("G10 L20 P1 X0Y0Z0"); GCodeDevice::getDevice()->scheduleCommand("G54"); }) );
        // menuItems.push_back(MenuItem{5, 'L', true, false, nullptr,
        //   [](MenuItem&){  GCodeDevice::getDevice()->scheduleCommand("M3 S1"); },
        //   [](MenuItem&){  GCodeDevice::getDevice()->scheduleCommand("M5"); } 
        // } );
    };


    void GrblDRO::drawContents() {
        const int LEN = 20;
        char str[LEN];

        GrblDevice *dev = static_cast<GrblDevice*>( GCodeDevice::getDevice() );
        if(dev==nullptr) return;

        U8G2 &u8g2 = Display::u8g2;

        u8g2.setDrawColor(1);
        u8g2.setFont(u8g2_font_7x13B_tr );

        int sx = 2;
        int sy = Display::STATUS_BAR_HEIGHT+5;

        //snprintf(str, 100, "u:%c bt:%d", digitalRead(PIN_DET)==0 ? 'n' : 'y',  buttStates);
        //u8g2.drawStr(sx, 7, str);
        //u8g2.drawGlyph(115, 0, !dev.isConnected() ? '-' : dev.isInPanic() ? '!' : '+' );

        snprintf(str, LEN, "X%8.3f", dev->getX() );   u8g2.drawStr(sx, sy, str);
        snprintf(str, LEN, "Y%8.3f", dev->getY() );   u8g2.drawStr(sx, sy+13, str);
        snprintf(str, LEN, "Z%8.3f", dev->getZ() );   u8g2.drawStr(sx, sy+26, str);

        snprintf(str, LEN, "S%d", dev->getSpindleVal() );   u8g2.drawStr(70, sy, str);
        snprintf(str, LEN, "F%d", dev->getFeed() );   u8g2.drawStr(70, sy+13, str);                
    };