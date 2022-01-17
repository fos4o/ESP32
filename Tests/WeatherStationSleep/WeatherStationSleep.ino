/*
 Name:		TestSensors.ino
 Created:	12/18/2019 10:37:54 PM
 Author:	fos4o
*/

#include "SoftwareSerial.h"
#include <MHZ19.h>
#include <ThingSpeak.h>
#include "driver/adc.h"
#include <esp_wifi.h>

#include <Wire.h>
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"

#include <MovingAverage.h>
#include <WiFi.h>
#include <Adafruit_Sensor.h>
#include <SparkFunBME280.h>
//#include <Adafruit_Si7021.h>
//#include <Adafruit_CCS811.h>
#include <MAX44009.h>

#include <TFT_eSPI.h>
#include "esp_adc_cal.h"

//#define DRAWONDISPLAY
#define PRINTONSERIAL

const uint32_t sleepTime = 5 * 1000;
const uint8_t WIFISendInterval = 20;
const uint8_t VoltagePin = GPIO_NUM_34;
int vref = 1100;

struct Data
{
	float m_WIFIStrength;

	float m_TempSIAvg;
	float m_PressBMPAvg;
	float m_HumSIAvg;

	//	float m_TempSiAvg;
	//	float m_HumSiAvg;

	float m_LuxMAXAvg;

	float m_CO2MHZ19;

	//float m_CO2CCSAvg;
	//float m_TVOCCCSAvg;

	float m_VBatteryAvg;

	uint32_t m_WIFIProcessingTime;

	uint8_t m_updateCnt;

	uint8_t channel = 255;
	uint8_t bssid[6];
	//uint8_t padding;

	void ResetData()
	{
		m_TempSIAvg = 0;
		m_PressBMPAvg = 0;
		m_HumSIAvg = 0;
		//		m_TempSiAvg = 0;
		//		m_HumSiAvg = 0;
		m_LuxMAXAvg = 0;
		m_CO2MHZ19 = 0;
		//m_CO2CCSAvg = 0;
		//m_TVOCCCSAvg = 0;
		m_VBatteryAvg = 0;
		m_updateCnt = 0;
		channel = 255;
	}
};

struct DataStore
{
	Data m_Data;
	uint32_t m_CRC;

	uint32_t calculateCRC32(const uint8_t* data, size_t length)
	{
		uint32_t crc = 0xffffffff;
		while (length--)
		{
			uint8_t c = *data++;
			for (uint32_t i = 0x80; i > 0; i >>= 1)
			{
				bool bit = crc & 0x80000000;
				if (c & i)
				{
					bit = !bit;
				}
				crc <<= 1;
				if (bit)
				{
					crc ^= 0x04c11db7;
				}
			}
		}
		return crc;
	}

	bool CheckCRC()
	{
		return (m_CRC == calculateCRC32((uint8_t*)&m_Data, sizeof(Data)));
	}

	void UpdateCRC()
	{
		m_CRC = calculateCRC32((uint8_t*)&m_Data, sizeof(Data));
	}

	bool ReadData()
	{
		//if (ESP.rtcUserMemoryRead(0, (uint32_t*)this, sizeof(DataStore)))
		//{
			if (CheckCRC())
			{
				return true;
			}
		//}
		return false;
	}

	bool WriteData()
	{
		UpdateCRC();
		return true;
		//return (ESP.rtcUserMemoryWrite(0, (uint32_t*)this, sizeof(DataStore)));
	}
};


BME280 m_BME280;
bool BMEFound = false;
//Adafruit_Si7021 m_Si7021;
//bool SIFound = false; 
MAX44009 m_MAX44009;
bool MAXFound = false;
//bool CCSFound = false;

float tempBME = 0;
float pressBME = 0;
float humBME = 0;

//float tempSi = 0;
//float humSi = 0;

float luxMAX = 0;

float batV = 0;

//float CO2CCS = 0;
//float TVOCCCS = 0;

float CO2MHZ19 = 0;
float tempMHZ19 = 0;

float vBat = 0;

//MHZ m_MZH(Serial2, MHZ19B);
MHZ19 m_MHZ19;

//Adafruit_CCS811 m_CCS811;

#ifdef DRAWONDISPLAY

TFT_eSPI display = TFT_eSPI();

#endif // DISPLAY


RTC_DATA_ATTR DataStore dataStore;

//I2C device found at address 0x3C  !
//I2C device found at address 0x40  !
//I2C device found at address 0x4A  !
//I2C device found at address 0x76  !

float fmap(float x, float in_min, float in_max, float out_min, float out_max)
{
	float res = (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
	return min(max(res, out_min), out_max);
}

void setup()
{
	unsigned long tm = millis();
	
	//WiFi.setSleep(true);
	//WiFi.mode(WIFI_OFF);
	//btStop();
	
#ifdef PRINTONSERIAL
	Serial.begin(115200);
	Serial.println("---------------------------------------------------------------------------");
	print_wakeup_reason();
#endif

	pinMode(VoltagePin, INPUT);
	Serial2.begin(9600, SERIAL_8N1, 32, 33);

	m_MHZ19.begin(Serial2);
	m_MHZ19.setRange(5000);


	Wire.begin();
	m_BME280.setI2CAddress(0x76);
	BMEFound = m_BME280.beginI2C();
	if (!BMEFound)
	{
#ifdef PRINTONSERIAL
		Serial.println("Could not find BME280");
#endif
	}
	m_BME280.setMode(MODE_FORCED);

	/*	SIFound = m_Si7021.begin();
		if (!SIFound)
		{
	#ifdef PRINTONSERIAL
			Serial.println("Could not find Si7021");
	#endif
		}*/

	MAXFound = m_MAX44009.begin() == 0;
	if (!MAXFound)
	{
#ifdef PRINTONSERIAL
		Serial.println("Could not find MAX44009");
#endif
	}

#ifdef DRAWONDISPLAY
	display.init();
	display.setRotation(3);

	display.setTextFont(2);
	display.setTextSize(1);
#endif
	/*CCSFound = m_CCS811.begin();
	if (CCSFound)
	{
		while (!m_CCS811.available());
	}
	else
	{
#ifdef PRINTONSERIAL
		Serial.println("Could not find CCS811");
#endif
	}

	while (!m_CCS811.available());

	float temp = m_CCS811.calculateTemperature();
	m_CCS811.setTempOffset(temp - 25.0);*/

	if (!dataStore.ReadData())
	{
#ifdef PRINTONSERIAL
		Serial.println("Error reading RTC data");
#endif
		dataStore.m_Data.ResetData();
		dataStore.m_Data.m_WIFIProcessingTime = 0;
	}
	else
	{
		//PrintRTCData();
	}

	if (BMEFound)
	{

		tempBME = m_BME280.readTempC();
		pressBME = m_BME280.readFloatPressure();
		humBME = m_BME280.readFloatHumidity();
	}

	CO2MHZ19 = m_MHZ19.getCO2(true, false);
	tempMHZ19 = m_MHZ19.getTemperature(true, false);

	/*if (CCSFound)
	{
		m_CCS811.setEnvironmentalData(humBME, tempBME);
		for (int i = 0; i < 10; i++)
		{
			if (m_CCS811.available())
			{
				if (!m_CCS811.readData())
				{
					CO2CCS = m_CCS811.geteCO2();
					if (CO2CCS > 0)
					{
						break;
					}
				}
			}
			delay(500);
		}
	}*/

	/*	if (SIFound)
		{
			tempSi = m_Si7021.readTemperature();
			humSi = m_Si7021.readHumidity();
		}*/

	if (MAXFound)
	{
		luxMAX = m_MAX44009.get_lux();
	}

	/*esp_adc_cal_characteristics_t adc_chars;
	esp_adc_cal_value_t val_type = esp_adc_cal_characterize((adc_unit_t)ADC_UNIT_1, (adc_atten_t)ADC1_CHANNEL_6, (adc_bits_width_t)ADC_WIDTH_BIT_12, 1100, &adc_chars);
	//Check type of calibration value used to characterize ADC
	if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) 
	{
		Serial.printf("eFuse Vref:%u mV", adc_chars.vref);
		vref = adc_chars.vref;
	}
	else if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) 
	{
		Serial.printf("Two Point --> coeff_a:%umV coeff_b:%umV\n", adc_chars.coeff_a, adc_chars.coeff_b);
	}
	else 
	{
		Serial.println("Default Vref: 1100mV");
	}*/

	//delay(5000);
	int analog = analogRead(VoltagePin);
	batV = ((float)analog / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);
	//batV *= 2.193;

	PrintDataOnDisplay();
	PrintMeasuredData();

	if (dataStore.m_Data.m_updateCnt >= WIFISendInterval)
	{
		WIFISend();
		dataStore.m_Data.ResetData();
	}

	dataStore.m_Data.m_updateCnt++;
	dataStore.m_Data.m_TempSIAvg += tempBME;
	dataStore.m_Data.m_PressBMPAvg += pressBME;
	dataStore.m_Data.m_HumSIAvg += humBME;
	//	dataStore.m_Data.m_TempSiAvg += tempSi;
	//	dataStore.m_Data.m_HumSiAvg += humSi;
	dataStore.m_Data.m_LuxMAXAvg += luxMAX;
	dataStore.m_Data.m_CO2MHZ19 += CO2MHZ19;
	//dataStore.m_Data.m_CO2CCSAvg += CO2CCS;

	dataStore.m_Data.m_VBatteryAvg += batV;

	if (dataStore.WriteData())
	{
#ifdef PRINTONSERIAL
		Serial.print("Data written ");
		Serial.print(dataStore.m_Data.m_updateCnt);
		Serial.println();
#endif
	}
	else
	{
#ifdef PRINTONSERIAL
		Serial.println("Error writing RTC data");
#endif
	}

	tm = millis() - tm;
	if(!Serial)
	{
		Serial.begin(115200);
		delay(10);
	}
	Serial.print("Processing Time ");
	Serial.print(tm);
	////Serial.print(" Data Size ");
	////Serial.print(sizeof(DataStore));
	Serial.println();

	//display.dim(true);
	
	adc_power_off();

	esp_sleep_enable_timer_wakeup(sleepTime * 1000);
	delay(1);
	esp_deep_sleep_start();

	/*if (dataStore.m_Data.m_updateCnt != WIFISendInterval)
	{
		ESP.deepSleep(sleepTime * 1000, RFMode::RF_DISABLED);
	}
	else
	{
		ESP.deepSleep(sleepTime * 1000);
	}*/

}

void WIFISend()
{
	unsigned long wifiTime = millis();
	WiFi.setSleep(false);
	WiFi.mode(WIFI_STA);
	WiFi.persistent(false);

	/*if (dataStore.m_Data.channel != 255)
	{
#ifdef PRINTONSERIAL
		Serial.print("ds channel: ");
		Serial.print(dataStore.m_Data.channel);
		Serial.println();
#endif

		WiFi.begin("hralupka", "namashetoneta", dataStore.m_Data.channel, dataStore.m_Data.bssid);
	}
	else*/
	{
		WiFi.begin("hralupka", "namashetoneta");
	}
#ifdef PRINTONSERIAL
	Serial.print("Connecting to WIFI ...");
#endif
	PrintWifiStartOnDisplay();

	/*IPAddress staticIP(192, 168, 1, 101);
	IPAddress gateway(192, 168, 1, 1);
	IPAddress subnet(255, 255, 255, 0);
	IPAddress dns1(77, 70, 15, 129);
	IPAddress dns2(89, 190, 192, 2);

	WiFi.config(staticIP, gateway, subnet, dns1, dns2);*/

	unsigned int tm = millis();
	while (WiFi.status() != WL_CONNECTED)
	{
#ifdef PRINTONSERIAL
		Serial.print(".");
#endif
		PrintStringOnDisplay(".");
		delay(250);
		if (millis() - tm > 10000)
		{
#ifdef PRINTONSERIAL
			Serial.println(" Can't Connect to WIFI");
#endif
			PrintStringOnDisplay(" Can't Connect to WIFI");
			break;
		}
	}

	Serial.println();

	if (WiFi.status() == WL_CONNECTED)
	{
		const char* server = "api.thingspeak.com";
		const String apiKey = "GX8I0YP983TGQK8S";


#ifdef PRINTONSERIAL
		Serial.println("WiFi connected");
		Serial.print("IP address: ");
		Serial.println(WiFi.localIP());
		Serial.print("Channel: ");
		Serial.print(WiFi.channel());
		Serial.print(" MAC: ");
		Serial.print(WiFi.macAddress());
		Serial.println();

#endif
		dataStore.m_Data.channel = WiFi.channel();
		memcpy(dataStore.m_Data.bssid, WiFi.BSSID(), 6);

		WiFiClient client;

		if (client.connect(server, 80))
		{
			PrintSendingOnDisplay();
#ifdef PRINTONSERIAL
			Serial.println("Sending data...");
#endif
			String postStr = apiKey;
			postStr += "&field1=";
			postStr += String(dataStore.m_Data.m_TempSIAvg / WIFISendInterval);
			postStr += "&field2=";
			postStr += String((dataStore.m_Data.m_PressBMPAvg / WIFISendInterval) / 100.0);
			postStr += "&field3=";
			postStr += String(dataStore.m_Data.m_HumSIAvg / WIFISendInterval);
			postStr += "&field4=";
			postStr += String(dataStore.m_Data.m_LuxMAXAvg / WIFISendInterval);
			postStr += "&field5=";
			postStr += String(dataStore.m_Data.m_CO2MHZ19 / WIFISendInterval);
			postStr += "&field6=";
			postStr += String(dataStore.m_Data.m_VBatteryAvg / WIFISendInterval);
			postStr += "&field7=";
			postStr += String(dataStore.m_Data.m_WIFIProcessingTime);
			postStr += "&field8=";
			postStr += String(dataStore.m_Data.m_WIFIStrength);
			postStr += "\r\n\r\n";

			PrintStringOnDisplay(".");

			dataStore.m_Data.m_WIFIStrength = fmap(WiFi.RSSI(), -100, -40, 0, 1) * 100.0;

			client.print("POST /update HTTP/1.1\n");
			PrintStringOnDisplay(".");

			client.print("Host: " + String(server) + "\n");
			PrintStringOnDisplay(".");
			client.print("Connection: close\n");
			PrintStringOnDisplay(".");
			client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
			PrintStringOnDisplay(".");
			client.print("Content-Type: application/x-www-form-urlencoded\n");
			PrintStringOnDisplay(".");
			client.print("Content-Length: ");
			PrintStringOnDisplay(".");
			client.print(postStr.length());
			PrintStringOnDisplay(".");
			client.print("\r\n\r\n");
			PrintStringOnDisplay(".");
			client.print(postStr);
			PrintStringOnDisplay(".");

#ifdef PRINTONSERIAL
			Serial.println("Data sent");
#endif
			delay(1500);
			client.stop();
		}

	}

	PrintStringOnDisplay(".");
	WiFi.disconnect(true);
	WiFi.setSleep(true);
	WiFi.mode(WIFI_OFF);

	delay(2);

	dataStore.m_Data.m_WIFIProcessingTime = millis() - wifiTime;
}

void PrintRTCData()
{
#ifdef PRINTONSERIAL
	Serial.print("Data read: ");
	Serial.print(dataStore.m_Data.m_updateCnt);
	float div = dataStore.m_Data.m_updateCnt + 1;
	Serial.print(" tBME: ");
	Serial.print(dataStore.m_Data.m_TempSIAvg / div);
	Serial.print(" pBME: ");
	Serial.print((dataStore.m_Data.m_PressBMPAvg / div) / 100.0f);
	Serial.print(" hBME: ");
	Serial.print(dataStore.m_Data.m_HumSIAvg / div);
	//Serial.print(" tSi: ");
	//Serial.print(dataStore.m_Data.m_TempSiAvg / div);
	//Serial.print(" hSi: ");
	//Serial.print(dataStore.m_Data.m_HumSiAvg / div);
	Serial.print(" lMAX: ");
	Serial.print(dataStore.m_Data.m_LuxMAXAvg / div);
	Serial.print(" CO2: ");
	Serial.print(dataStore.m_Data.m_CO2MHZ19 / div);
	Serial.print(" VBat: ");
	Serial.print(dataStore.m_Data.m_VBatteryAvg / div);
	Serial.print(" WIFI Time: ");
	Serial.print(dataStore.m_Data.m_WIFIProcessingTime);
	Serial.println();
#endif
}

void PrintMeasuredData()
{
#ifdef PRINTONSERIAL
	Serial.print(" tBME: ");
	Serial.print(tempBME);
	Serial.print(" pBME: ");
	Serial.print(pressBME / 100);
	Serial.print(" hBME: ");
	Serial.print(humBME);

	//Serial.print(" tSI: ");
	//Serial.print(tempSi);
	//Serial.print(" hSI: ");
	//Serial.print(humSi);

	Serial.print(" CO2MHZ19: ");
	Serial.print(CO2MHZ19);
	Serial.print(" tempMHZ19: ");
	Serial.print(tempMHZ19);

	//Serial.print(" CO2CCS: ");
	//Serial.print(CO2CCS);

	Serial.print(" lMAX: ");
	Serial.print(luxMAX);

	Serial.print(" batV ");
	Serial.print(batV);

	Serial.print(" WIFITime: ");
	Serial.print(dataStore.m_Data.m_WIFIProcessingTime);

	Serial.print(" WIFIStrength: ");
	Serial.print(dataStore.m_Data.m_WIFIStrength);

	Serial.println();
#endif
}

void PrintWifiStartOnDisplay()
{
#ifdef DRAWONDISPLAY
	display.fillScreen(TFT_BLACK);
	display.setTextColor(TFT_GREEN);
	display.setCursor(0, 0);
	display.print("Connecting to WIFI.");
#endif
}

void PrintSendingOnDisplay()
{
#ifdef DRAWONDISPLAY
	display.fillScreen(TFT_BLACK);
	display.setTextColor(TFT_YELLOW);
	display.setCursor(0, 0);
	display.print("Sending Data.");
#endif
}

void PrintStringOnDisplay(String str)
{
#ifdef DRAWONDISPLAY
	display.print(str);
#endif
}

void PrintStringOnDisplay(uint16_t x, uint16_t y, String str)
{
#ifdef DRAWONDISPLAY
	display.setCursor(0, 0);
	display.print(str);
#endif
}

void PrintDataOnDisplay()
{
#ifdef DRAWONDISPLAY
	
	if (dataStore.m_Data.m_updateCnt == 0)
	{
		return;
	}

	float div = dataStore.m_Data.m_updateCnt;
	char buf[64];

	display.fillScreen(TFT_BLACK);
	display.setTextColor(TFT_RED);
	display.setCursor(0, 0);
	
	sprintf(buf, "Temperature:%8.2f", dataStore.m_Data.m_TempSIAvg / div);
	display.print(buf);
	display.println();

	display.setTextColor(display.color565(50, 128, 255));
	sprintf(buf, "Humidity:%12.2f", dataStore.m_Data.m_HumSIAvg / div);
	display.print(buf);
	display.println();

	display.setTextColor(display.color565(108, 229, 255));
	sprintf(buf, "Pressure:%11.2f", dataStore.m_Data.m_PressBMPAvg * 0.01 / div);
	display.print(buf);
	display.println();

	display.setTextColor(display.color565(255, 255, 50));
	sprintf(buf, "Light:%15.2f", dataStore.m_Data.m_LuxMAXAvg / div);
	display.print(buf);
	display.println();

	display.setTextColor(display.color565(255, 255, 50));
	sprintf(buf, "CO2:%17.2f", dataStore.m_Data.m_CO2MHZ19 / div);
	display.print(buf);
	display.println();

	display.setTextColor(display.color565(100, 255, 50));
	sprintf(buf, "WIFI Strength:%7.0f", dataStore.m_Data.m_WIFIStrength);
	display.print(buf);
	display.println();

	display.setTextColor(display.color565(255, 100, 50));
	sprintf(buf, "Battery:%13.2f", dataStore.m_Data.m_VBatteryAvg / div);
	display.print(buf);
	display.println();

	display.setTextColor(TFT_WHITE);
	sprintf(buf, "CntSnd:%3i /%3i", dataStore.m_Data.m_updateCnt, WIFISendInterval);
	display.print(buf);
	display.println();

	delay(3000);
#endif
}

void print_wakeup_reason() 
{
#ifdef PRINTONSERIAL
	esp_sleep_wakeup_cause_t wakeup_reason;

	wakeup_reason = esp_sleep_get_wakeup_cause();

	switch (wakeup_reason)
	{
		case ESP_SLEEP_WAKEUP_EXT0: Serial.println("Wakeup caused by external signal using RTC_IO"); break;
		case ESP_SLEEP_WAKEUP_EXT1: Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
		case ESP_SLEEP_WAKEUP_TIMER: Serial.println("Wakeup caused by timer"); break;
		case ESP_SLEEP_WAKEUP_TOUCHPAD: Serial.println("Wakeup caused by touchpad"); break;
		case ESP_SLEEP_WAKEUP_ULP: Serial.println("Wakeup caused by ULP program"); break;
		default: Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason); break;
	}
#endif
}

void loop()
{
}