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

WatchedSerial SerialCNC{Serial1, PIN_DET};

U8G2 &Display::u8g2 = _u8g2;
// todo work with Pin_detect
// // GrblDevice dev{&SerialCNC, PIN_DET};

uint8_t devBuf[MAX(sizeof(GrblDevice), sizeof(MarlinDevice))];
uint8_t droBuf[MAX(sizeof(GrblDRO), sizeof(MarlinDRO))];

Display display;

GCodeDevice *dev;
Job job;
DRO *dro;

FileChooser fileChooser;

GCodeDevice* createGrbl(WatchedSerial *s) {
    if (dev != nullptr) return dev;

    GrblDevice *device = new(devBuf) GrblDevice(s);

    delay(1000);
    device->begin();
    device->add_observer(*Display::getDisplay());
    device->add_observer(job);

    dro = new (droBuf) GrblDRO(*device);
    dro->begin();
    dro->enableRefresh();
    display.setScreen(dro);
    display.setDevice(device);
    job.setDevice(device);

    dev = device;
    return dev;
}

using Detector = GrblDetector<WatchedSerial, SerialCNC, createGrbl>;

DetectorScreen<Detector> detUI;

void setup() {
    SerialUSB.begin(115200);
    //SerialCNC.begin(115200);

    _u8g2.begin();
    _u8g2.setFontPosTop();
    _u8g2.setFontMode(1);
    _u8g2.setDrawColor(1);

    display.begin();

    display.setScreen(&detUI);

    fileChooser.setCallback( [&](bool res, String path){
        if(res) {
            LOGF("Starting job %s\n", path.c_str() );
            job.setFile(path);
            job.start();   // TODO what happens on job stop
        } else {
            // cancel
        }
        Display::getDisplay()->setScreen(dro);
    } );
    fileChooser.begin();

    job.add_observer( display );

    for(auto pin: buttPins) {
        pinMode(pin, INPUT_PULLUP);
    }

    Detector::init();
#if LOG_DEBUG
    File cDir = SD.open("/");
    File file;
    while ( (file = cDir.openNextFile()) ) {
        LOGF("file %s\n", file.name() );
        file.close();
    }
    cDir.close();
#endif  //LOG_DEBUG
}

void loop() {
    static uint32_t nextRead;
    // poll buttons
    if( int32_t(millis() - nextRead) > 0) {
        for(int i=0; i<N_BUTT; i++) {
            bitWrite(display.buttStates, i, (digitalRead(buttPins[i]) == 0 ? 1 : 0));
        }
        // display.processInput();
        nextRead = millis() + 10;
    }
    //END poll buttons
    display.loop();
    job.loop();

    if (dev != nullptr) {
        dev->loop();
    } else {
        Detector::loop();
    }
    // todo check and fix
//    if(SerialUSB.available()) {
//        while(SerialUSB.available()) {
//            SerialCNC.write(SerialUSB.read());
//        }
//    }

}

