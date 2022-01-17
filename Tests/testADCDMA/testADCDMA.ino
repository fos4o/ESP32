#include <Arduino.h>
#include "driver/i2s.h"
#include "driver/adc.h"
#include "esp_event_loop.h"
#include "math.h"

const uint32_t    SAMPLE_RATE = 10000;
const uint32_t    NUM_SAMPLES = 1024;

const uint32_t    BUF_SAMP = 20000;
uint16_t accBuf[BUF_SAMP];
int accBufCnt;

static QueueHandle_t i2s_event_queue;

int64_t times;

const int ledPin = GPIO_NUM_27;
const int freq = 100;
const int ledChannel = 0;
const int resolution = 8;


void setup() 
{
	//ledcSetup(ledChannel, freq, resolution);
	//ledcAttachPin(ledPin, ledChannel);

	//ledcWrite(ledChannel, 64);

	esp_err_t esp_timer_init();

	Serial.begin(115200);

	i2s_config_t i2s_config;
	i2s_config.mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN);
	i2s_config.sample_rate = SAMPLE_RATE;
	i2s_config.dma_buf_len = NUM_SAMPLES;
	i2s_config.channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT;
	i2s_config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
	i2s_config.use_apll = false,
	i2s_config.communication_format = I2S_COMM_FORMAT_I2S;
	i2s_config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
	i2s_config.dma_buf_count = 2;
	i2s_driver_install(I2S_NUM_0, &i2s_config, 1, &i2s_event_queue);
	
	i2s_set_adc_mode(ADC_UNIT_1, ADC1_CHANNEL_6);

	analogSetWidth(12);
	analogSetCycles(8);
	analogSetClockDiv(1);
	analogSetAttenuation(ADC_11db);

	i2s_adc_enable(I2S_NUM_0);
	times = esp_timer_get_time();

	accBufCnt = 0;
}

void loop()
{
	uint16_t i2s_read_buff[NUM_SAMPLES];
	size_t sizeRead = 0;
	system_event_t evt;
	
	BaseType_t xStatus;
	xStatus = xQueueReceive(i2s_event_queue, &evt, portMAX_DELAY);
	//Serial.println(xStatus);
	if (xStatus == pdPASS)
	{
		if (evt.event_id == 2)
		{
			int64_t timee = esp_timer_get_time();
			Serial.print((double(NUM_SAMPLES) / double(timee - times)) * 1000000.0);

			i2s_adc_disable(I2S_NUM_0);
			i2s_read(I2S_NUM_0, (char*)i2s_read_buff, NUM_SAMPLES * sizeof(uint16_t), &sizeRead, portMAX_DELAY);
			i2s_adc_enable(I2S_NUM_0);
			times = esp_timer_get_time();
			int size = min(BUF_SAMP - accBufCnt, NUM_SAMPLES);
			memcpy(((char*)accBuf + accBufCnt * sizeof(uint16_t)), i2s_read_buff, size * sizeof(uint16_t));

/*			for (int i = 0; i < size; i++)
			{
				accBuf[i + accBufCnt] = i2s_read_buff[i];
			}*/
			uint32_t sum = 0;
			if (accBufCnt + NUM_SAMPLES >= BUF_SAMP)
			{
				accBufCnt = 0;
				for (int i = 0; i < BUF_SAMP; i++)
				{
					Serial.println(accBuf[i]);
					sum += accBuf[i];
				}
			}
			else
			{
				accBufCnt += size;
			}
			Serial.print(" ");
			Serial.print((((float)sum / BUF_SAMP) / 4095.0) * 2.0 * 3.3 * 1.1);
			Serial.println();
		}
	}
}