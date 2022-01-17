/*
 Name:		GY_21PTest.ino
 Created:	2/9/2019 5:41:30 PM
 Author:	fos4o
*/

// the setup function runs once when you press reset or power the board


#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_Si7021.h>
#include <Adafruit_CCS811.h>

#include <Adafruit_GFX.h>

Adafruit_BMP280 m_BMP280;
Adafruit_Si7021 m_Si7021;
Adafruit_CCS811 m_CCS811;


void setup()
{
	Serial.begin(115200);
	if (!m_BMP280.begin(0x76))
	{
		Serial.println("Could not find BMP280");
		while (true);
	}

	if (!m_Si7021.begin())
	{
		Serial.println("Could not find Si7021");
		while (true);
	}

	if (!m_CCS811.begin())
	{
		Serial.println("Could not find CCS811");
		while (true);
	}

	while (!m_CCS811.available());

	float temp = m_CCS811.calculateTemperature();
	m_CCS811.setTempOffset(temp - 25.0);

}

// the loop function runs over and over again until power down or reset
void loop()
{
	float tempBMP = m_BMP280.readTemperature();
	float pressBMP = m_BMP280.readPressure();
	//float altBMP = m_BMP280.readTemperature();

	float humSI = m_Si7021.readHumidity();
	float tempSI = m_Si7021.readTemperature();

	Serial.print("Temp = ");
	Serial.print(tempBMP);
	Serial.println(" *C");
	Serial.print("Pressure = ");
	Serial.print(pressBMP);
	Serial.println(" Pa");
	Serial.print("Approx altitude = ");
	//Serial.print(m_BMP280.readAltitude(1013.25)); // this should be adjusted to your local forcase
	Serial.print(m_BMP280.readAltitude(1020.7));
	Serial.println(" m");
	Serial.println();

	Serial.println("SI7021 results");
	Serial.print("Humidity: ");
	Serial.println(humSI, 2);
	Serial.print("Temperature: ");
	Serial.println(tempSI, 2);
	Serial.println();


	if (m_CCS811.available())
	{
		m_CCS811.setEnvironmentalData(humSI, tempSI);
		//float temp = m_CCS811.calculateTemperature();
		if (!m_CCS811.readData())
		{
			Serial.print("CO2: ");
			Serial.print(m_CCS811.geteCO2());
			Serial.print("ppm, TVOC: ");
			Serial.print(m_CCS811.getTVOC());
			//Serial.print("ppb   Temp:");
			//Serial.print(temp);
			Serial.println("");
		}
		else
		{
			Serial.println("ERROR!");
		}
	}

	delay(1000);
}
