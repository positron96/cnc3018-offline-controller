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

        

        //u8g2.setFont(u8g2_font_nokiafc22_tr);
        //u8g2.drawGlyph(64, 7, cMode==Mode::AXES ? 'M' : 'S');  

        u8g2.setFont(u8g2_font_7x13B_tr );

        int sx = 2;
        int sy = Display::STATUS_BAR_HEIGHT+1, sx2=72;

        //snprintf(str, 100, "u:%c bt:%d", digitalRead(PIN_DET)==0 ? 'n' : 'y',  buttStates);
        //u8g2.drawStr(sx, 7, str);
        //u8g2.drawGlyph(115, 0, !dev.isConnected() ? '-' : dev.isInPanic() ? '!' : '+' );

        u8g2.setDrawColor(1);
        u8g2.drawFrame((cMode==Mode::AXES ? sx : sx2)-2,  sy-1, cMode==Mode::AXES?70:50, 13*3+1);

        //u8g2.setDrawColor(1);

        snprintf(str, LEN, "X%8.3f", dev->getX() );   u8g2.drawStr(sx, sy, str);
        snprintf(str, LEN, "Y%8.3f", dev->getY() );   u8g2.drawStr(sx, sy+13, str);
        snprintf(str, LEN, "Z%8.3f", dev->getZ() );   u8g2.drawStr(sx, sy+26, str);

        sx2 += 5;
        snprintf(str, LEN, "S %d", dev->getSpindleVal() );   
        u8g2.drawStr(sx2, sy, str);
        snprintf(str, LEN, "F %d", JOG_FEEDS[cFeed] );   
        u8g2.drawStr(sx2, sy+13, str);  
        snprintf(str, LEN, JOG_DISTS[cDist]<1?"D %.1f":"d %.0f", JOG_DISTS[cDist] );   
        u8g2.drawStr(sx2, sy+26, str);  

    };

    
    void GrblDRO::onButton(int bt, Display::ButtonEvent evt) {
        
        GrblDevice *dev = static_cast<GrblDevice*>( GCodeDevice::getDevice() );
        if(dev==nullptr) return;

        S_DEBUGF("GrblDRO::onButton(%d,%d)\n", bt, (int)evt);
        
        if(bt==Display::BT_CENTER) {
            switch(evt) {
                case Evt::DOWN: {
                    cMode = cMode==Mode::AXES ? Mode::SPINDLE : Mode::AXES;
                    buttonWasPressedWithShift = false;
                } 
                break;
                case Evt::UP: {
                    if(buttonWasPressedWithShift) { cMode = cMode==Mode::AXES ? Mode::SPINDLE : Mode::AXES; }
                }
                break;
            }
            setDirty();
            return;
        } 
        
        if(evt==Evt::DOWN) buttonWasPressedWithShift = true;

        if (bt==Display::BT_STEP && evt==Evt::DOWN) {
            dev->scheduleCommand("$X"); 
            return;
        } 

        if(cMode == Mode::AXES) {
            onButtonAxes(bt, evt);
        } else {
            onButtonShift(bt, evt);
        }
        
    };

    void GrblDRO::onButtonAxes(int bt, Evt evt) {
        if(evt==Evt::DOWN || evt==Evt::HOLD) {
            GrblDevice *dev = static_cast<GrblDevice*>( GCodeDevice::getDevice() );

            int axis=-1;
            float d = JOG_DISTS[cDist];
            int f = JOG_FEEDS[cFeed];

            switch(bt) {              
                case Display::BT_L:     axis=0; d = -d; break;
                case Display::BT_R:     axis=0; break;
                case Display::BT_UP:    axis=1; break;
                case Display::BT_DOWN:  axis=1; d = -d; break;
                case Display::BT_ZUP:   axis=2; break;
                case Display::BT_ZDOWN: axis=2; d = -d; break;
                default: break;
            }
            if(axis!=-1) {
                dev->jog(axis, d, f);
                setDirty();
            }
        }
    }

    void GrblDRO::onButtonShift(int bt, Evt evt) {
        GrblDevice *dev = static_cast<GrblDevice*>( GCodeDevice::getDevice() );

        if(! (evt==Evt::DOWN || evt==Evt::HOLD) ) return;

        switch(bt) {
            case Display::BT_UP: 
                if(cDist<N_JOG_DISTS-1) cDist++;
                break;
            case Display::BT_DOWN: 
                if(cDist>0) cDist--;
                break;
            case Display::BT_L: 
            case Display::BT_R:  {
                if(bt==Display::BT_L && cSpindleVal<N_SPINDLE_VALS-1) cSpindleVal++;
                if(bt==Display::BT_R && cSpindleVal>0) cSpindleVal--;
                int v = SPINDLE_VALS[cSpindleVal];
                if(v!=0) {
                    char t[15];
                    snprintf(t, 15, "M3 S%d", v);
                    dev->scheduleCommand(t);
                } else {
                    dev->scheduleCommand("M5"); 
                }
                break;
            }
            case Display::BT_ZDOWN:
                if(cFeed>0)cFeed--;
                break;
            case Display::BT_ZUP: //dev->schedulePriorityCommand("?");  break;
                if(cFeed<N_JOG_FEEDS-1) cFeed++;
                break;
        }
        
    }