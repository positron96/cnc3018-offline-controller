#include <U8g2lib.h>
#include <SD.h>
#include "debug.h"
#include "constants.h"

U8G2_SSD1306_128X64_NONAME_F_4W_SW_SPI _u8g2(
        U8G2_R0,
        PIN_LCD_CLK,
        PIN_LCD_MOSI,
        PIN_LCD_CS,
        PIN_LCD_DC,
        PIN_LCD_RST
);

#include "WatchedSerial.h"

#include "ui/DetectorScreen.h"
#include "ui/GrblDRO.h"
#include "ui/MarlinDRO.h"
#include "ui/FileChooser.h"
#include "ui/Display.h"

#include "devices/DeviceDetector.h"

#include "devices/GrblDevice.h"
#include "devices/MarlinDevice.h"

#include "Job.h"

constexpr uint32_t buttPins[N_BUTT] = {
        PIN_BT_ZDOWN,
        PIN_BT_ZUP,

        PIN_BT_R,
        PIN_BT_L,
        PIN_BT_CENTER,
        PIN_BT_UP,
        PIN_BT_DOWN,

        PIN_BT_STEP
};

U8G2& Display::u8g2 = _u8g2;
WatchedSerial serialCNC{Serial1, PIN_DET};


uint8_t devBuf[MAX(sizeof(GrblDevice), sizeof(MarlinDevice))];
uint8_t droBuf[MAX(sizeof(GrblDRO), sizeof(MarlinDRO))];

Job job;
Display display{job};

GCodeDevice* dev;
DRO* dro;

FileChooser fileChooser;

void createGrbl(int i, WatchedSerial* s) {
    if (dev != nullptr) return;
    delay(500);
    switch (i) {
        case 0: {
            GrblDevice* device = new(devBuf) GrblDevice(s, &job);
            dro = new(droBuf) GrblDRO(*device);
            dev = device;
        }
            break;
        default: {
            MarlinDevice* device = new(devBuf) MarlinDevice(s, &job);
            dro = new(droBuf) MarlinDRO(*device);
            dev = device;
        }
    }
    display.setScreen(dro);
    display.setDevice(dev);
    job.setDevice(dev);
    dev->begin();
    dev->add_observer(*Display::getDisplay());
    dro->begin();
    dro->enableRefresh();
    LOGLN("Created");
}

using Detector = GrblDetector<WatchedSerial, serialCNC, createGrbl>;
DetectorScreen<Detector> detUI;

void setup() {
    SerialUSB.begin(115200);
    _u8g2.begin();
    _u8g2.setFontPosTop();
    _u8g2.setFontMode(1);
    _u8g2.setDrawColor(1);

    display.begin();

    display.setScreen(&detUI);

    fileChooser.setCallback([](bool res, const char* path) {
        if (res) {
            LOGF("Starting job %s\n", path);
            job.setFile(path);
            job.start();
        }
        Display::getDisplay()->setScreen(dro);
    });
    fileChooser.begin();

    job.add_observer(display);

    for (auto pin: buttPins) {
        pinMode(pin, INPUT_PULLUP);
    }

    Detector::init();
}

void loop() {
    static uint32_t nextRead;
    // poll buttons
    if (int32_t(millis() - nextRead) > 0) {
        for (int i = 0; i < N_BUTT; i++) {
            bitWrite(display.buttStates, i, (digitalRead(buttPins[i]) == 0 ? 1 : 0));
        }
        display.processInput();
        nextRead = millis() + 20;
    }
    //END poll buttons

    display.step();
    if (dev != nullptr) {
        job.step();
        dev->step();
    } else {
        Detector::loop();
    }

#ifdef LOG_DEBUG
    //send all data from pc to device
   if(SerialUSB.available()) {
       while(SerialUSB.available()) {
           serialCNC.write(SerialUSB.read());
       }
   }
#endif
}

