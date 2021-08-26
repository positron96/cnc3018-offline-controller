#include "DRO.h"


    void DRO::drawContents() {
        const int LEN = 20;
        char str[LEN];

        GCodeDevice *dev = GCodeDevice::getDevice();
        if(dev==nullptr) return;

        U8G2 &u8g2 = Display::u8g2;

        u8g2.setFont( u8g2_font_7x13B_tr );
        int y = Display::STATUS_BAR_HEIGHT+3, h=u8g2.getAscent()-u8g2.getDescent()+3;

        //u8g2.drawGlyph(0, y+h*(int)cAxis, '>' ); 

        u8g2.setDrawColor(1);
        u8g2.drawBox(0, y+h*(int)cAxis-1, 8, h);

        u8g2.setDrawColor(2);

        drawAxis('X', dev->getX(), y); y+=h;
        drawAxis('Y', dev->getY(), y); y+=h;
        drawAxis('Z', dev->getZ(), y); y+=h;        

        y+=5;
        u8g2.setFont( u8g2_font_nokiafc22_tr   );
        float m = distVal(cDist);
        snprintf(str, LEN, m<1 ? "x%.1f" : "x%.0f", m );
        u8g2.drawStr(0, y, str);
                
    };


    struct POT_CFG { int MX; int N; int D;};

    static constexpr POT_CFG POTS_CFG[]{ {1000, 3, 50}, {1200,3,50} };
    
    void DRO::onPotValueChanged(int pot, int v) {
        //  center lines : 2660    3480    4095
        // borders:            3000    3700
        // v1:    0     250     500
        //          125    375

        const POT_CFG &p = POTS_CFG[pot];
        const int l = p.MX / p.N;
        const int d = POTS_CFG[pot].D;
        int &var = pot==0 ? cAxis : cDist;
        bool ch=false;
        
        int rangeL = var * l - d;
        int rangeH = rangeL + l + 2*d;
        if(v<rangeL && var>0    ) { var--; ch=true; }
        if(v>rangeH && var<p.N-1) { var++; ch=true; }
        if(ch && pot==0) lastJogTime=0;
         if(ch) {
            //S_DEBUGF("changed pot: axis:%d dist:%d, pot%d=%d\n", (int)cAxis, (int)cDist, pot, v);
            setDirty();
        } 
    }


    void DRO::onButtonPressed(Button bt, int8_t arg) {
        GCodeDevice *dev = GCodeDevice::getDevice();
        if(dev==nullptr) {
            S_DEBUGF("device is null\n");
            return;
        }
        switch(bt) {
            case Button::ENC_UP:
            case Button::ENC_DOWN: {
                if(! dev->canJog() ) return;
                float f=0;
                float d = distVal(cDist)*arg;
                if( lastJogTime!=0) { f = d / (millis()-lastJogTime) * 1000*60; };
                if(f<500) f=500;
                //S_DEBUGF("jog af %d, dt=%d ms, delta=%d\n", (int)f, millis()-lastJog, arg);
                bool r = dev->jog( (int)cAxis, d, (int)f );
                lastJogTime = millis();
                if(!r) S_DEBUGF("Could not schedule jog\n");
                setDirty();
                break;
            }
            default: break;
        }
    };
