#include "ZProbeSizeScreen.h"
#include "GrblDRO.h"

extern GrblDRO dro;

void ZProbeSizeScreen::begin(Screen *screen) {
    returnScreen = screen;
    setDirty();
}

void ZProbeSizeScreen::drawContents() {

    U8G2 &u8g2 = Display::u8g2;

    u8g2.setDrawColor(1);

    u8g2.setFont(u8g2_font_nokiafc22_tr);
    const char *title = "Set Z probe size";
    int titleWidth = u8g2.getStrWidth(title);
    u8g2.drawStr((u8g2.getWidth() - titleWidth) / 2, 15, title);

    char buffer[16];

    uint16_t integerPart = probeSize100 / 100;
    uint16_t decimalPart = probeSize100 % 100;

    snprintf(buffer, sizeof(buffer), "%02u.%02u mm", integerPart, decimalPart);

    u8g2.setFont(u8g2_font_helvB14_tr);
    int valueWidth = u8g2.getStrWidth(buffer);
    u8g2.drawStr((u8g2.getWidth() - valueWidth) / 2, 29, buffer);

    u8g2.setFont(u8g2_font_nokiafc22_tr);
    const char *help1 = "UP/DN: .01  L/R: .10 or .50";
    int helpWidth = u8g2.getStrWidth(help1);
    u8g2.drawStr((u8g2.getWidth() - helpWidth) / 2, 50, help1);
}


void ZProbeSizeScreen::onButton(int bt, Evt evt) {
    if(evt != Evt::DOWN && evt != Evt::HOLD)
        return;

    switch(bt) {
        /*
         * +0.01 mm
         */
        case Display::BT_UP:
            if(probeSize100 < 9999)
                probeSize100++;

            setDirty();
            break;

        /*
         * -0.01 mm
         */
        case Display::BT_DOWN:
            if(probeSize100 > 0)
                probeSize100--;

            setDirty();
            break;

        /*
         * -0.10 mm or -0.50 mm
         */
        case Display::BT_L:
            if(evt == Evt::HOLD) {
                if(probeSize100 >= 50)
                    probeSize100 -= 50;
                else
                    probeSize100 = 0;
            }
            else {
                if(probeSize100 >= 10)
                    probeSize100 -= 10;
                else
                    probeSize100 = 0;
            }

            setDirty();
            break;

        /*
         * +0.10 mm or +0.50 mm
         */
        case Display::BT_R:
            if(evt == Evt::HOLD) {
                if(probeSize100 <= 9949)
                    probeSize100 += 50;
                else
                    probeSize100 = 9999;
            }
            else {
                if(probeSize100 <= 9989)
                    probeSize100 += 10;
                else
                    probeSize100 = 9999;
            }

            setDirty();
            break;

        case Display::BT_STEP:
            if(returnScreen != nullptr) {
                Display::getDisplay()->setScreen(returnScreen);
                Display::getDisplay()->showMenu();
            }
            break;

        default:
            break;
    }
}