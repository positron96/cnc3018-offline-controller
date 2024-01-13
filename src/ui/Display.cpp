#include "Display.h"
#include <Arduino.h>
#include "Screen.h"

#include "../devices/GrblDevice.h"

constexpr int VISIBLE_MENUS = 5;

Display *Display::inst = nullptr;

uint16_t Display::buttStates;

Display *Display::getDisplay() { return inst; }

void Display::setScreen(Screen *screen) {
    if (cScreen != nullptr)
        cScreen->onHide();
    cScreen = screen;
    if (cScreen != nullptr)
        cScreen->onShow();
    selMenuItem = 0;
    menuShown = false;
    dirty = true;
}

void Display::setDevice(GCodeDevice *de) {
    dev = de;
}

void Display::loop() {
    if (cScreen != nullptr)
        cScreen->loop();
    draw();
}

void Display::ensureSelMenuVisible() {
    if (selMenuItem >= cScreen->firstDisplayedMenuItem + VISIBLE_MENUS)
        cScreen->firstDisplayedMenuItem = selMenuItem - VISIBLE_MENUS + 1;
    if (selMenuItem < cScreen->firstDisplayedMenuItem) {
        cScreen->firstDisplayedMenuItem = selMenuItem;
    }
}

void Display::processInput() {
    processButtons();
}

void Display::processButtons() {
    decltype(buttStates) changed = buttStates ^ prevStates;

    if (cScreen == nullptr) return;
    ButtonEvent evt;
    for (int i = 0; i < N_BUTTONS; i++) {
        bool down = bitRead(buttStates, i);
        if (bitRead(changed, i)) {
            if (i == BT_STEP && down && !cScreen->menuItems.empty()) {
                menuShown = !menuShown;
                setDirty();
            } else {
                evt = down ? ButtonEvent::DOWN : ButtonEvent::UP;
                if (down) menuShownWhenDown = menuShown;
                // don't propagate events to screen if the click was in the menu
                if (menuShown) {
                    processMenuButton(i, evt);
                } else if (!menuShownWhenDown) {
                    cScreen->onButton(i, evt);
                }
            }
            holdCounter[i] = 0;
        } else if (down) {
            holdCounter[i]++;
            if (holdCounter[i] == HOLD_COUNT) {
                evt = ButtonEvent::HOLD;
                if (menuShown) {
                    processMenuButton(i, evt);
                } else {
                    cScreen->onButton(i, evt);
                }
                holdCounter[i] = 0;
            }
        }
    }
    prevStates = buttStates;
}

void Display::processMenuButton(uint8_t bt, ButtonEvent evt) {
    if (!(evt == ButtonEvent::DOWN || evt == ButtonEvent::HOLD)) return;
    size_t menuLen = cScreen->menuItems.size();
    if (menuLen != 0) {
        if (bt == BT_UP) {
            selMenuItem = selMenuItem > 0 ? selMenuItem - 1 : menuLen - 1;
            ensureSelMenuVisible();
            setDirty();
        }
        if (bt == BT_DOWN) {
            selMenuItem = (selMenuItem + 1) % menuLen;
            ensureSelMenuVisible();
            setDirty();
        }
        if (bt == BT_CENTER) {
            MenuItem &item = cScreen->menuItems[selMenuItem];
            if (!item.togglable) { item.on = !item.on; }
            item.cmd(item);
            menuShown = false;
            setDirty();
        }
    }
}

void Display::draw() {
    if (!dirty)
        return;
    u8g2.clearBuffer();
    if (cScreen != nullptr)
        cScreen->drawContents();

    drawStatusBar();
    if (menuShown)
        drawMenu();

    u8g2.sendBuffer();
    dirty = false;
}

#include "../assets/locked.XBM"
#include "../assets/connected.XBM"

void Display::drawStatusBar() {
    constexpr int LEN = 26;
    char str[LEN];

    if (dev == nullptr)
        return;

    u8g2.setFont(u8g2_font_nokiafc22_tr);
    // allow 2 lines in status bar
    constexpr int fH = 8; // 8x8
    int x = 2, y = -1;

    if (!dev->isConnected()) {
        u8g2.drawGlyph(x, y, 'X');
    } else if (dev->isLocked()) {
        u8g2.drawXBM(x, 0, locked_width, locked_height, (const uint8_t *) locked_bits);
    } else {
        u8g2.drawXBM(x, 0, connected_width, connected_height, (const uint8_t *) connected_bits);
    }
    memcpy(str, dev->getStatusStr(), LEN);
    str[LEN-1] = 0;
    u8g2.drawStr(LEN, y + fH, str);
    //job status
    if (job.isValid()) {
        uint8_t p = job.getCompletion();
        snprintf(str, 9, "% 2d%%", p);
        if (job.isPaused())
            str[0] = 'P';
        else
            str[0] = 'R';
        str[9] = 0;
        u8g2.setDrawColor(1);
        u8g2.drawStr(48, y, str);
    }
}

void Display::drawMenu() {
    if (cScreen == nullptr) return;
    u8g2.setFont(u8g2_font_nokiafc22_tr);

    const size_t len = cScreen->menuItems.size();

    size_t onscreenLen = len - cScreen->firstDisplayedMenuItem;
    if (onscreenLen > VISIBLE_MENUS)
        onscreenLen = VISIBLE_MENUS;
    const int w = 80, x = 20, lh = 8, h = onscreenLen * lh;
    int y = 6;

    u8g2.setDrawColor(0);
    u8g2.drawBox(x, y, w, lh + h + 4);
    u8g2.setDrawColor(1);
    u8g2.drawFrame(x, y, w, lh + h + 4);

    char str[20];
    snprintf(str, 20, "Menu [%d/%d]", selMenuItem + 1, len);
    u8g2.drawStr(x + 2, y + 1, str);

    y = 16;

    for (size_t i = 0; i < onscreenLen; i++) {
        size_t idx = cScreen->firstDisplayedMenuItem + i;
        if (selMenuItem == idx) {
            u8g2.setDrawColor(1);
            u8g2.drawBox(x, y + i * lh, w, lh);
        }
        MenuItem &item = cScreen->menuItems[idx];
        if (item.font != nullptr) u8g2.setFont(item.font);
        u8g2.setDrawColor(2);
        u8g2.drawStr(x + 2, y + i * lh - 1, item.text.c_str());
    }

    u8g2.setDrawColor(0);
    int xx = x + w - 5;
    u8g2.drawBox(xx - 1, y, 5, h);
    u8g2.setDrawColor(1);
    u8g2.drawFrame(xx, y, 5, h);
    if (len > VISIBLE_MENUS) {
        const uint hh = (h - 2) * VISIBLE_MENUS / len;
        const uint sy = (h - hh) * cScreen->firstDisplayedMenuItem / (len - VISIBLE_MENUS);
        u8g2.drawBox(xx + 1, y + 1 + sy, 3, hh);
    }
}
