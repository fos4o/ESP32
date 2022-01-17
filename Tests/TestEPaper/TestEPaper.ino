#include <GxEPD.h>

// select the display class to use, only one
#include <GxGDEP015OC1/GxGDEP015OC1.h>    // 1.54" b/w
//#include <GxGDEW0213I5F/GxGDEW0213I5F.h>  // 2.13" b/w 104x212 flexible
//#include <GxGDE0213B1/GxGDE0213B1.h>      // 2.13" b/w
//#include <GxGDEH0213B72/GxGDEH0213B72.h>  // 2.13" b/w new panel
//#include <GxGDEH0213B73/GxGDEH0213B73.h>  // 2.13" b/w newer panel
//#include <GxGDEH029A1/GxGDEH029A1.h>      // 2.9" b/w
//#include <GxGDEW029T5/GxGDEW029T5.h>      // 2.9" b/w IL0373
//#include <GxGDEW026T0/GxGDEW026T0.h>      // 2.6" b/w
//#include <GxGDEW027W3/GxGDEW027W3.h>      // 2.7" b/w
//#include <GxGDEW0371W7/GxGDEW0371W7.h>    // 3.7" b/w
//#include <GxGDEW042T2/GxGDEW042T2.h>      // 4.2" b/w
//#include <GxGDEW075T7/GxGDEW075T7.h>      // 7.5" b/w 800x480
// these displays do not fully support partial update
//#include <GxGDEW0154Z17/GxGDEW0154Z17.h>  // 1.54" b/w/r 152x152
//#include <GxGDEW0213Z16/GxGDEW0213Z16.h>  // 2.13" b/w/r
//#include <GxGDEW029Z10/GxGDEW029Z10.h>    // 2.9" b/w/r
//#include <GxGDEW027C44/GxGDEW027C44.h>    // 2.7" b/w/r
//#include <GxGDEW042Z15/GxGDEW042Z15.h>    // 4.2" b/w/r
//#include <GxGDEW0583T7/GxGDEW0583T7.h>    // 5.83" b/w
//#include <GxGDEW075T8/GxGDEW075T8.h>      // 7.5" b/w
//#include <GxGDEW075Z09/GxGDEW075Z09.h>    // 7.5" b/w/r
//#include <GxGDEW075Z08/GxGDEW075Z08.h>    // 7.5" b/w/r 800x480

#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

// FreeFonts from Adafruit_GFX
#include <Fonts/FreeMonoBold9pt7b.h>

#include GxEPD_BitmapExamples

// for SPI pin definitions see e.g.:
// C:\Users\xxx\AppData\Local\Arduino15\packages\esp8266\hardware\esp8266\2.4.2\variants\generic\common.h

GxIO_Class io(SPI, /*CS=5*/ SS, /*DC=*/ 22, /*RST=*/ 21); // arbitrary selection of 17, 16
GxEPD_Class display(io, /*RST=*/ 21, /*BUSY=*/ 15); // arbitrary selection of (16), 4



//#define DEMO_DELAY 3*60 // seconds
//#define DEMO_DELAY 1*60 // seconds
#define DEMO_DELAY 30

void setup(void)
{
    Serial.begin(74880);
    Serial.println();
    Serial.println("setup");
    display.init(74880); // enable diagnostic output on Serial
    Serial.println("setup done");
}

void loop()
{
    showPartialUpdatePaged();
    delay(DEMO_DELAY * 1000);
}

void showBlackBoxCallback(uint32_t v)
{
    uint16_t box_x = 10;
    uint16_t box_y = 15;
    uint16_t box_w = 70;
    uint16_t box_h = 20;
    display.fillRect(box_x, box_y, box_w, box_h, v);
}

void showValueBoxCallback(const uint32_t v)
{
    uint16_t box_x = 10;
    uint16_t box_y = 15;
    uint16_t box_w = 70;
    uint16_t box_h = 20;
    uint16_t cursor_y = box_y + box_h - 6;
    display.fillRect(box_x, box_y, box_w, box_h, GxEPD_WHITE);
    display.setCursor(box_x, cursor_y);
    display.print(v / 100);
    display.print(v % 100 > 9 ? "." : ".0");
    display.print(v % 100);
}

void showPartialUpdatePaged()
{
    uint16_t box_x = 10;
    uint16_t box_y = 15;
    uint16_t box_w = 70;
    uint16_t box_h = 20;
    uint16_t cursor_y = box_y + box_h - 6;
    uint32_t value = 1395;
    display.setFont(&FreeMonoBold9pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.setRotation(0);
    // draw background
    display.drawExampleBitmap(BitmapExample1, sizeof(BitmapExample1));
    delay(2000);

    // partial update to full screen to preset for partial update of box window
    // (this avoids strange background effects)
    display.drawExampleBitmap(BitmapExample1, sizeof(BitmapExample1), GxEPD::bm_default | GxEPD::bm_partial_update);
    delay(1000);

    // show where the update box is
    for (uint16_t r = 0; r < 4; r++)
    {
        display.setRotation(r);
        display.drawPagedToWindow(showBlackBoxCallback, box_x, box_y, box_w, box_h, GxEPD_BLACK);
        delay(1000);
        display.drawPagedToWindow(showBlackBoxCallback, box_x, box_y, box_w, box_h, GxEPD_WHITE);
    }

    // show updates in the update box
    for (uint16_t r = 0; r < 4; r++)
    {
        // reset the background
        display.drawExampleBitmap(BitmapExample1, sizeof(BitmapExample1), GxEPD::bm_default | GxEPD::bm_partial_update);
        display.setRotation(r);
        for (uint16_t i = 1; i <= 10; i++)
        {
            uint32_t v = value * i;
            display.drawPagedToWindow(showValueBoxCallback, box_x, box_y, box_w, box_h, v);
            delay(500);
        }
        delay(1000);
        display.drawPagedToWindow(showBlackBoxCallback, box_x, box_y, box_w, box_h, GxEPD_WHITE);
    }
    // reset the background
    display.drawExampleBitmap(BitmapExample1, sizeof(BitmapExample1), GxEPD::bm_default | GxEPD::bm_partial_update);
    display.setRotation(0);
    display.powerDown();
}