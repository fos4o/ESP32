#pragma once
#include "SampleSource.h"
class SinWave :
    public SampleSource
{

private:
    int m_freq;

    uint16_t *dataSin;
    uint16_t *dataCos;
    uint m_dataSize;
    uint idx;

public:
    SinWave();
    ~SinWave();

    void initData(uint32_t size);

    void SetFrequency(int freq);

    uint32_t frequency();

    void getFrames(Frame_t* frames, int number_frames);
};

