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

        u8g2.setFont(u8g2_font_nokiafc22_tr);
        
        u8g2.drawGlyph(64, 7, cMode==Mode::AXES ? 'M' : 'S');  

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
        snprintf(str, LEN, "F%d", JOG_FEEDS[cFeed] );   u8g2.drawStr(70, sy+13, str);  
        snprintf(str, LEN, JOG_DISTS[cDist]<1?"d %.1f":"d %.0f", JOG_DISTS[cDist] );   u8g2.drawStr(70, sy+26, str);  

    };

    
    void GrblDRO::onButton(int bt, int8_t arg) {
        GrblDevice *dev = static_cast<GrblDevice*>( GCodeDevice::getDevice() );
        if(dev==nullptr) return;
        if(!arg) return;
        if(bt==Display::BT_CENTER) {
            //dev->schedulePriorityCommand("?"); 
            if(cMode==Mode::AXES) cMode = Mode::SPINDLE; else cMode = Mode::AXES;
        } else if (bt==Display::BT_STEP) {
            dev->scheduleCommand("$X"); 
        } else {
            if(cMode == Mode::AXES) {

                float d = JOG_DISTS[cDist];
                int f = JOG_FEEDS[cFeed];
                //if( lastJogTime!=0) { f = d / (millis()-lastJogTime) * 1000*60; };
                //if(f<500) f=500;
                //S_DEBUGF("jog af %d, dt=%d ms, delta=%d\n", (int)f, millis()-lastJog, arg);
                //lastJogTime = millis();

                switch(bt) {              
                    case Display::BT_L: dev->jog(0, -d, f); break;
                    case Display::BT_R: dev->jog(0, d, f); break;
                    case Display::BT_UP: dev->jog(1, d, f); break;
                    case Display::BT_DOWN: dev->jog(1, -d, f); break;
                    case Display::BT_ZUP: dev->jog(2, d, f); break;
                    case Display::BT_ZDOWN: dev->jog(2, -d, f); break;
                    default: break;
                }
            } else {
                switch(bt) {
                    case Display::BT_UP: 
                        if(cDist<N_JOG_DISTS-1) cDist++;
                        break;
                    case Display::BT_DOWN: 
                        if(cDist>0) cDist--;
                        break;
                    case Display::BT_R: dev->scheduleCommand("M3 S255"); break;
                    case Display::BT_L: dev->scheduleCommand("M5"); break;
                    case Display::BT_ZDOWN:
                        if(cFeed>0)cFeed--;
                        break;
                    case Display::BT_ZUP: //dev->schedulePriorityCommand("?");  break;
                        if(cFeed<N_JOG_FEEDS-1) cFeed++;
                        break;
                }
            }
        }
        
    };