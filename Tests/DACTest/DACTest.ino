#include <Arduino.h>

#include <SPIFFS.h>

#include "WAVFileReader.h"
#include "DACOutput.h"
#include "SinWave.h"

DACOutput *dac;
SinWave *sWave;
WAVFileReader* wReader;

// the setup function runs once when you press reset or power the board
void setup() 
{
    Serial.begin(115200);
    delay(500);
    Serial.println("---------------------------------------------------------------------------");

    Serial.println("Starting up");

    SPIFFS.begin();

    Serial.println("Created sample source");

    //wReader = new WAVFileReader("/sine.wav");
    sWave = new SinWave();

    dac = new DACOutput();
    //dac->start(wReader);
    dac->start(sWave);
}

// the loop function runs over and over again until power down or reset
void loop() 
{

}