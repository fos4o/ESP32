#include "driver/adc.h"
#include "esp_adc_cal.h"

// Get CPU cycle counter (240MHz)
uint32_t IRAM_ATTR cycles()
{
	uint32_t ccount;
	__asm__ __volatile__("rsr     %0, ccount" : "=a" (ccount));
	return ccount;
}

#define SAMPLES 20000
uint16_t val[SAMPLES];

const int pin = GPIO_NUM_34;

const int ledPin = GPIO_NUM_27;
const int freq = 500;
const int ledChannel = 0;
const int resolution = 8;


void setup()
{
	Serial.begin(115200);

	pinMode(GPIO_NUM_34, INPUT);

	//ledcSetup(ledChannel, freq, resolution);
	//ledcAttachPin(ledPin, ledChannel);

	analogSetWidth(12);
	analogSetCycles(8);
	analogSetClockDiv(1);
	analogSetAttenuation(ADC_11db);

	adc1_config_width(ADC_WIDTH_BIT_12);
	adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11);

	adcAttachPin(pin);
}

void loop()
{
	int i;

	unsigned long tms = millis();

	//ledcWrite(ledChannel, 64);

	for (i = 0; i < SAMPLES; i++)
	{
		adcStart(pin);
		val[i] = adcEnd(pin);
		//val[i] = analogRead(pin);
		//val[i] = adc1_get_raw(ADC1_CHANNEL_6);
	}
	unsigned long tme = millis();

	uint32_t sample = 0;
	for (i = 0; i < SAMPLES; i++)
	{
		//Serial.println(val[i]);
		sample += val[i];
	}

	esp_adc_cal_characteristics_t* adc_chars = (esp_adc_cal_characteristics_t*)calloc(1, sizeof(esp_adc_cal_characteristics_t));
	esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_0, ADC_WIDTH_BIT_12, 1100, adc_chars);
	//Check type of calibration value used to characterize ADC
	if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
		Serial.println("eFuse Vref");
	}
	else if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
		Serial.println("Two Point");
	}
	else {
		Serial.println("Default");
	}

	uint32_t voltage = esp_adc_cal_raw_to_voltage((float)sample / SAMPLES, adc_chars);
	Serial.print((((float)sample / SAMPLES) / 4096.0) * 2.0 * 3.3 * 1.1);
	Serial.print(" ");
	Serial.print(voltage * 2 * 3.3 * 0.001);
	Serial.print(" ");
	Serial.println((float)(SAMPLES) / ((tme - tms) / 1000.0f));
}
