#include "Display.h"

#include <Arduino.h>

#include "Screen.h"

// #define D_DEBUGF(...)  { Serial.printf(__VA_ARGS__); }
// #define D_DEBUGFI(...)  { log_printf(__VA_ARGS__); }
// #define D_DEBUGS(s)  { Serial.println(s); }

Display * Display::inst = nullptr;


uint16_t Display::buttStates;


    Display* Display::getDisplay() { return inst; }

    void Display::setScreen(Screen *screen) { 
        if(cScreen != nullptr) cScreen->onHide();
        cScreen = screen; 
        if(cScreen != nullptr) cScreen->onShow();
        selMenuItem = 0;
        dirty=true;
    }


    void Display::loop() {
        if(cScreen!=nullptr) cScreen->loop();
        draw();
    }

    constexpr int VISIBLE_MENUS = 6;

    void Display::ensureSelMenuVisible() {

        if(selMenuItem >= cScreen->firstDisplayedMenuItem+VISIBLE_MENUS)
           cScreen->firstDisplayedMenuItem = selMenuItem - VISIBLE_MENUS + 1;

        if(selMenuItem < cScreen->firstDisplayedMenuItem) {
            cScreen->firstDisplayedMenuItem = selMenuItem;
        }
    }

    void Display::processInput() {
        processButtons();
    } 


    void Display::processButtons() {
        
        decltype(buttStates) changed = buttStates ^ prevStates;

        if (cScreen == nullptr) return;
        for(int i=0; i<N_BUTTONS; i++) {
            bool down = bitRead(buttStates, i);
            if(bitRead(changed, i) ) {
                cScreen->onButton(i, down ? ButtonEvent::DOWN : ButtonEvent::UP);
                holdCounter[i] = 0;
            } else if(down) {
                holdCounter[i]++;
                if(holdCounter[i] == HOLD_COUNT) {
                    cScreen->onButton(i, ButtonEvent::HOLD);
                    holdCounter[i] = 0;
                }
            }
        }
        prevStates = buttStates;
        

        // static const Button buttons[] = {Button::BT1, Button::BT2, Button::BT3};
        // if (cScreen == nullptr) return;
        // for(int bt=0; bt<3; bt++) {
        //     bool p = buttonPressed[bt];
        //     if(lastButtPressed[bt] != p) {
        //         S_DEBUGF("button%d changed: %d\n", bt, p );
        //         if(p) {
        //             int menuLen = cScreen->menuItems.size();
        //             if(menuLen!=0) {
        //                 if(bt==0) { selMenuItem = selMenuItem>0 ? selMenuItem-1 : menuLen-1; ensureSelMenuVisible(); setDirty(); }
        //                 if(bt==2) { selMenuItem = (selMenuItem+1) % menuLen; ensureSelMenuVisible(); setDirty(); }
        //                 if(bt==1) {
        //                     MenuItem& item = cScreen->menuItems[selMenuItem];
        //                     if(!item.togglalbe) { item.onCmd(item); }
        //                     else {
        //                         if(item.on) { item.offCmd(item); item.on=false; } else { item.onCmd(item); item.on=true; }
        //                     }
        //                 }
        //                 //cScreen->onMenuItemSelected(cScreen->menuItems[selMenuItem]);                    
        //             } else {
        //                 cScreen->onButton(buttons[bt], 1);
        //             }
        //         }
        //         lastButtPressed[bt] = p;
        //     }
        // }
    }

    void Display::draw() {
        if(!dirty) return;
        u8g2.clearBuffer();
        if(cScreen!=nullptr) cScreen->drawContents();
        drawStatusBar();
        drawMenu();

        //char str[15]; sprintf(str, "%lu", millis() ); u8g2.drawStr(20,20, str);
        //char str[15]; sprintf(str, "%4d %4d", potVal[0], potVal[1] ); u8g2.drawStr(5,110, str);
        //char str[15]; sprintf(str, "%d", encVal ); u8g2.drawStr(5,110, str);

        u8g2.sendBuffer();
        dirty = false;
    }

    void Display::drawStatusBar() {

        GrblDevice *dev = static_cast<GrblDevice*>( GCodeDevice::getDevice() );

        //u8g2.setFont(u8g2_font_5x8_tr);
        u8g2.setDrawColor(1);

        u8g2.setFont(u8g2_font_nokiafc22_tr);

        const int LEN=25;

        char str[LEN];
        //snprintf(str, 25, "DET:%c", digitalRead(PIN_DET)==0 ? '0' : '1' );
        if(dev==nullptr || !dev->isConnected()) {
            snprintf(str, LEN, "no conn");
        } else {
            if(dev->isInPanic()) {
                snprintf(str, LEN, "ALERT"); 
            } else if(dev->isLocked() ) {
                snprintf(str, LEN, "LOCK");
            } else {
                snprintf(str, LEN, "conn");
            }
        }
        u8g2.drawStr(2, -1, str );

        snprintf(str, LEN, dev->getStatus().c_str() ); 
        u8g2.drawStr(2, 7, str);  

        //snprintf(str, 100, "u:%c bt:%d", digitalRead(PIN_DET)==0 ? 'n' : 'y',  buttStates);
        //u8g2.drawStr(sx, 7, str);

        // job status
        // Job *job = Job::getJob();
        // char str[20];
        // if(job->isValid() ) {
        //     float p = job->getCompletion()*100;
        //     if(p<10) snprintf(str, 20, " %.1f%%", p );
        //     else snprintf(str, 20, " %d%%", (int)p );
        //     if(job->isPaused() ) str[0] = '|';
        // } else strncpy(str, " ---%", 20);
        // int w = u8g2.getStrWidth(str);
        // u8g2.drawStr(u8g2.getWidth()-w, 0, str);
        //S_DEBUGF("drawing '%s' len %d\n", str, strlen(str) );


        // line
        //u8g2.drawHLine(0, STATUS_BAR_HEIGHT-1, u8g2.getWidth() );
    }


    void Display::drawMenu() {
        // if(cScreen==nullptr) return;
        // u8g2.setFont(u8g2_font_5x8_tr);
        // u8g2.setDrawColor(2);
        
        // int len = cScreen->menuItems.size();

        // int onscreenLen = len - cScreen->firstDisplayedMenuItem;
        // if (onscreenLen>VISIBLE_MENUS) onscreenLen=VISIBLE_MENUS;
        // const int w=10;
        // int y = u8g2.getHeight()-w;        
        
        // for(int i=0; i<onscreenLen; i++) {
        //     int idx = cScreen->firstDisplayedMenuItem + i;
        //     if(selMenuItem == idx) {
        //         u8g2.drawBox(i*w, y, w, w);    
        //     } else {            
        //         u8g2.drawFrame(i*w, y, w, w);
        //     }
        //     MenuItem &item = cScreen->menuItems[idx];
        //     uint16_t c = item.glyph;
        //     if(item.font != nullptr) u8g2.setFont(item.font);
        //     u8g2.drawGlyph(i*w+2, y+1, c);
        // }
    }
    

