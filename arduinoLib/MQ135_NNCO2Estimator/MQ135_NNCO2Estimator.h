/****************************************************
This is an Arduino library for CO2-concentration estimation using MQ135 (cheap air-quality sensor)



MIT license
written by Kyohei Yamada, NITTC
*****************************************************/

#ifndef MQ135_NNCO2ESTIMATOR
#define MQ135_NNCO2ESTIMATOR

#if (ARDUINO >= 100)
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif


//time in milliseconds
#define msec_1sec  1000
#define msec_10sec 1000*10
#define msec_1min  1000*60
#define msec_10min 1000*60*10
#define msec_1h    1000*60*60
#define msec_24h   1000*60*60*24


class MQ135_NNCO2Estimator{
  struct DataBuffer{
    uint32_t intervalMillis;
    uint8_t length;
    uint32_t PrevUpdateTime;
    uint16_t *buffer;
  };
  class adcManager{
  public:
    adcManager();
    bool update(uint16_t datum);
    float secAverage(float timeRangeSec);
  private:
    float bufferAvg(DataBuffer const&buffer, uint32_t msecRange);
    bool pushBuffer(DataBuffer &buffer ,uint16_t datum);
    //バッファ
    uint16_t _1secBuff[10];
    uint16_t _10secBuff[10];
    uint16_t _1minBuff[6];
    uint16_t _10minBuff[10];
    uint16_t _1hBuff[6];
    uint16_t _24hBuff[24];

    DataBuffer  _adc1sec={100,10,0,_1secBuff};
    DataBuffer  _adc10sec={msec_1sec,10,0,_10secBuff};
    DataBuffer  _adc1min={msec_10sec,6,0,_1minBuff};
    DataBuffer  _adc10min={msec_1min,10,0,_10minBuff};
    DataBuffer  _adc1h={msec_10min,6,0,_1hBuff};
    DataBuffer  _adc24h={msec_1h,24,0,_24hBuff};
  };
  struct tempBuffer{
    uint32_t intervalMillis;
    uint8_t length;
    uint32_t PrevUpdateTime;
    int16_t *buffer;
  };
  class tempManager{
  public:
    tempManager();
    bool update(float datum);
    float secAverage(float timeRangeSec);
  private:
    float bufferAvg(tempBuffer const&buffer, int32_t msecRange);
    bool pushBuffer(tempBuffer &buffer ,int16_t datum);
    //バッファ
    int16_t _1secBuff[10];
    int16_t _10secBuff[10];
    int16_t _1minBuff[6];
    int16_t _10minBuff[10];
    int16_t _1hBuff[6];
    int16_t _24hBuff[24];

    tempBuffer  _temp1sec={100,10,0,_1secBuff};
    tempBuffer  _temp10sec={msec_1sec,10,0,_10secBuff};
    tempBuffer  _temp1min={msec_10sec,6,0,_1minBuff};
    tempBuffer  _temp10min={msec_1min,10,0,_10minBuff};
    tempBuffer  _temp1h={msec_10min,6,0,_1hBuff};
    tempBuffer  _temp24h={msec_1h,24,0,_24hBuff};
  };

public:
  MQ135_NNCO2Estimator(void);
  float estimate(void);
  adcManager adc;
  tempManager temperature;
  tempManager humidity;
private:
  float relu(float inputSum);
  float getWeight(uint8_t layer, uint8_t node, uint8_t output);
  float getBias(uint8_t layer, uint8_t node);
};

#endif //  MQ135_NNCO2ESTIMATOR
