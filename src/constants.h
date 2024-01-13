//
// Created by lima on 12/10/23.
//

#ifndef CNC_3018_CONSTANTS_H
#define CNC_3018_CONSTANTS_H

#include <Arduino.h>
// ====================== SD ======================
constexpr uint32_t PIN_CD = PA3;

// ===================== LCD =====================
constexpr uint32_t PIN_LCD_CS = PA2; // not connected
constexpr uint32_t PIN_LCD_RST = PB0;
constexpr uint32_t PIN_LCD_DC = PB1;
constexpr uint32_t PIN_LCD_CLK = PA0;
constexpr uint32_t PIN_LCD_MOSI = PA1;
// ===================== BUTTONS ==================
constexpr int N_BUTT = 8;
constexpr uint32_t PIN_BT_ZDOWN = PB8;
constexpr uint32_t PIN_BT_ZUP   = PB9;
constexpr uint32_t PIN_BT_R     = PB10;
constexpr uint32_t PIN_BT_L     = PB11;
constexpr uint32_t PIN_BT_CENTER= PB12;
constexpr uint32_t PIN_BT_UP    = PB13;
constexpr uint32_t PIN_BT_DOWN  = PB14;
constexpr uint32_t PIN_BT_STEP  = PB15;

constexpr uint32_t PIN_DET      = PC13;    ///< 0V=no USB on CNC, 1=CNC connected to USB.


constexpr char AXIS[] =     {'X', 'Y', 'Z' , 'E'};
constexpr char AXIS_WCS[] = {'x', 'y', 'z'};
constexpr char PRINTER[] =  { 'T', 'B'};

//================ PROTOCOL ==============
constexpr char XON =  0x11;
constexpr char XOFF = 0x13;


#endif //CNC_3018_CONSTANTS_H
