
#include <Arduino.h>
#include "driver/i2s.h"
#include <math.h>
#include <SPIFFS.h>
#include <FS.h>

#include "soc/rtc_wdt.h"
#include "esp_int_wdt.h"
#include "esp_task_wdt.h"

#include "SampleSource.h"
#include "DACOutput.h"

// number of frames to try and send at once (a frame is a left and right sample)

void i2sWriterTask(void* param)
{
    DACOutput* output = (DACOutput*)param;
    int availableBytes = 0;
    int buffer_position = 0;
    Frame_t *frames = new Frame_t[output->m_size];

    rtc_wdt_protect_off();
    rtc_wdt_disable();
    disableCore0WDT();
    disableCore1WDT();
    disableLoopWDT();

    while (true)
    {
        // wait for some data to be requested
        i2s_event_t evt;
        if (xQueueReceive(output->m_i2sQueue, &evt, portMAX_DELAY) == pdPASS)
        {
            if (evt.type == I2S_EVENT_TX_DONE)
            {
                size_t bytesWritten = 0;
                do
                {
                    if (availableBytes == 0)
                    {
                        // get some frames from the wave file - a frame consists of a 16 bit left and right sample
                        output->m_sample_generator->getFrames(frames, output->m_size);
                        // how many bytes do we now have to send
                        availableBytes = output->m_size * sizeof(uint32_t);
                        // reset the buffer position back to the start
                        buffer_position = 0;
                    }
                    // do we have something to write?
                    if (availableBytes > 0)
                    {
                        // write data to the i2s peripheral
                        i2s_write(I2S_NUM_0, buffer_position + (uint8_t*)frames,
                            availableBytes, &bytesWritten, 100);
                        availableBytes -= bytesWritten;
                        buffer_position += bytesWritten;
                        //Serial.print(availableBytes);
                        //Serial.print(" ");
                        //Serial.println(bytesWritten);
                    }
//                    delay(1);
                } while ((bytesWritten > 0) && (availableBytes > 0));
            }
        }
    }
}

void DACOutput::start(SampleSource* sample_generator)
{
    m_sample_generator = sample_generator;
    // i2s config for writing both channels of I2S
    float frequency = m_sample_generator->frequency();
    Serial.print("freq: ");
    Serial.println(frequency);

    m_size = 8;
    /*if (frequency < 350)
    {
        m_size = 512;
    }
    else */if (frequency < 640)
    {
        m_size = 256;
    }
    else if (frequency < 1500)
    {
        m_size = 128;
    }
    else if (frequency < 5000)
    {
        m_size = 64;
    }
    else if (frequency < 10000) 
    {
        m_size = 32;
    }
    else if (frequency < 20000) 
    {
        m_size = 16;
    }
    else 
    {
        m_size = 8;
    }
    
    Serial.print("size: ");
    Serial.println(m_size);

    m_rate = frequency * m_size;

    if (m_rate < 5200)
    {
        m_rate = 5200;
    }
    else if (m_rate > 650000)
    {
        m_rate = 650000;
    }

    //m_rate = 15200;

    Serial.print("rate: ");
    Serial.println(m_rate);

    frequency = m_rate / m_size;

    //m_size *= 2;


    Serial.print("freq2: ");
    Serial.println(frequency);

    sample_generator->initData(m_size);

    i2s_driver_uninstall(I2S_NUM_0);

    i2s_config_t i2sConfig =
    {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
        .sample_rate = m_rate,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_MSB,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 2,
        .dma_buf_len = m_size * 2,
        .tx_desc_auto_clear = true
    };
    
    //Serial.println(i2sConfig.sample_rate); 
    //install and start i2s driver
    esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2sConfig, 2, &m_i2sQueue);
    //Serial.println(err);
    ESP_ERROR_CHECK(i2s_set_sample_rates(I2S_NUM_0, m_rate));

    //ESP_ERROR_CHECK(i2s_set_clk(0, 8000, 16, I2S_CHANNEL_STEREO));
    // enable the DAC channels
    i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN);
    //Serial.println(err);
    // clear the DMA buffers
    i2s_zero_dma_buffer(I2S_NUM_0);
    //Serial.println(err);
    // start a task to write samples to the i2s peripheral
    TaskHandle_t writerTaskHandle;
    xTaskCreate(i2sWriterTask, "i2s Writer Task", 4096, this, 1, &writerTaskHandle);
}


