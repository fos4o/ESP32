#include "SinWave.h"


SinWave::SinWave()
{
	m_freq = 1000;
	m_dataSize = 0;
	dataSin = NULL;
	dataCos = NULL;
}

SinWave::~SinWave()
{
	delete dataSin;
	delete dataCos;
	dataSin = NULL;
	dataCos = NULL;
	m_dataSize = 0;
}

void SinWave::initData(uint32_t size)
{
	m_dataSize = size * 2;
	dataSin = new uint16_t[m_dataSize];
	dataCos = new uint16_t[m_dataSize];

	float step = (360.0f / size ) * (PI / 180.0f);
	float cal;
	float cStep = 0;
	for (int i = 0; i < m_dataSize; i++)
	{
		cal = sin(cStep);

		dataSin[i] = uint16_t(cal * 32767) + 32768;

		cal = cos(cStep);
		dataCos[i] = uint16_t(cal * 32767) + 32768;

		cStep += step;
	}
	idx = 0;
}

void SinWave::SetFrequency(int freq)
{
	m_freq = freq;
}

uint32_t SinWave::frequency()
{
	return m_freq;
}

void SinWave::getFrames(Frame_t * frames, int number_frames)
{
	for (int i = 0; i < number_frames; i++)
	{
		frames[i].left = dataSin[idx];
		frames[i].right = dataCos[idx];
		idx++;
		if(idx >= m_dataSize)
		{
			idx = 0;
		}
	}

	/*float step = (360.0f / number_frames) * (PI / 180.0f);
	float cal;
	float cStep = 0;
	for (int i = 0; i < number_frames; i++)
	{
		cal = sin(cStep);

		frames[i].left = uint16_t(cal * 32767) + 32768;

		cal = cos(cStep);
		frames[i].right = uint16_t(cal * 32767) + 32768;

		cStep += step;
	}*/
}