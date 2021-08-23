#include <U8g2lib.h>

#define PIN_LCD_CS PA2 // not connected
#define PIN_LCD_RST PB0
#define PIN_LCD_DC PB1
#define PIN_LCD_CLK PA0
#define PIN_LCD_MOSI PA1

U8G2_SSD1306_128X64_NONAME_F_4W_SW_SPI u8g2(U8G2_R0, PIN_LCD_CLK, PIN_LCD_MOSI, 
    PIN_LCD_CS, PIN_LCD_DC, PIN_LCD_RST);

constexpr uint32_t PIN_BT_CENTER = PB12;
constexpr uint32_t PIN_BT_UP = PB13;
constexpr uint32_t PIN_BT_DOWN = PB14;
constexpr uint32_t PIN_BT_L = PB11;
constexpr uint32_t PIN_BT_R = PB10;

HardwareSerial Serial1(PA10, PA9);

void setup() {
    //SystemClock_Config();
    Serial1.begin(115200);
    SerialUSB.begin(115200);

    u8g2.begin();
    //u8g2.setBusClock(600000);
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setFontPosTop();
    u8g2.setFontMode(1);
    u8g2.setDrawColor(1);

    pinMode(PIN_BT_CENTER, INPUT_PULLUP);
    pinMode(PIN_BT_UP, INPUT_PULLUP);
    pinMode(PIN_BT_DOWN, INPUT_PULLUP);
    pinMode(PIN_BT_L, INPUT_PULLUP);
    pinMode(PIN_BT_R, INPUT_PULLUP);
}

constexpr int sp=5;
int x=64, y=32;
int dx=sp,dy=sp;
bool animate=true;

constexpr int r=5;
constexpr int ROW1 = 16;


void loop() {

    static uint32_t nextRead;

    if( int32_t(millis() - nextRead) > 0) {
        if(digitalRead(PIN_BT_CENTER)==0) animate = !animate;
        if(digitalRead(PIN_BT_DOWN)==0) y+=sp;
        if(digitalRead(PIN_BT_UP)==0) y-=sp;    
        if(digitalRead(PIN_BT_R)==0) x+=sp;
        if(digitalRead(PIN_BT_L)==0) x-=sp; 
        nextRead = millis() + 100;

    }

    static uint32_t lastRedraw, lastFps;
    static int frames, lastFrames;
    if(millis()-lastRedraw > 100) {
        lastRedraw = millis();

        u8g2.clearBuffer();
        u8g2.setDrawColor(1);
        u8g2.drawFilledEllipse(x, y, r,r);
        //u8g2.setDrawColor(0);
        //u8g2.drawFilledEllipse(30, 30, 15,15);
        char str[100];
        snprintf(str, 100, "%d/%d", x,y);
        u8g2.setDrawColor(1);
        u8g2.drawStr(10, 0, str);

        snprintf(str, 100, "FPS:%d", lastFrames);
        u8g2.drawStr(60, 0, str);

        frames++;
        if(millis()-lastFps>1000) {
            lastFrames = frames;
            frames=0;
            lastFps = millis();

            Serial1.println(str);
        }

        u8g2.sendBuffer();
    }

    //SerialUSB.println(str);

    //i = (i+1)%128;
    static uint32_t lastRecalc;
    if(animate && millis()-lastRecalc>25) {
        x += dx;
        y += dy;
        if(x>=128-r) dx = -sp;
        if(y>=64-r) dy = -sp;
        if(x<=r) dx=sp;
        if(y<=ROW1+r) dy=sp;
        lastRecalc = millis();
    }

    //delay(1);
}