#include <Arduino.h>

#include <SPIFFS.h>

#include "DACOutput.h"
#include "SinWave.h"

DACOutput* dac;
SinWave* sWave;

// the setup function runs once when you press reset or power the board
void setup()
{
    Serial.begin(115200);
    delay(500);
    Serial.println("---------------------------------------------------------------------------");

    Serial.println("Starting up");

    SPIFFS.begin();

    Serial.println("Created sample source");

    sWave = new SinWave();
    //sWave->SetFrequency(1000);

    dac = new DACOutput();
    dac->start(sWave);
}

// the loop function runs over and over again until power down or reset
void loop()
{

}