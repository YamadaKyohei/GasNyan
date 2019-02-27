
#include "MQ135_NNCO2Estimator.h"
/*
DataBuffer::DataBuffer(uint8_t length, uint8_t intervalMillis){
  _buffer
}
*/

//中間層の数+入力層+出力層
#define depth 3+2
#define MaxNode 4
//各層のノード数 {入力層…中間層…出力層}
const uint8_t nodeNum[depth] = {4,4,4,4,1};

//const PROGMEM float w[depth-1][MaxNode][MaxNode]  = {{1,2},{3,4}};
const float w[depth-1][MaxNode][MaxNode] = {
  {{-0.44467396, -0.23543301,  0.01691875, -0.03211159},
   {-0.68741124, -0.4740332 ,  0.23518413, -0.0321148 },
   { 0.02151973,  0.08404239,  0.49543178, -0.03213056},
   { 1.08488325,  0.70434441, -0.7666048 , -0.03232015}},
  {{  2.26380946e-001,   4.01975520e-001,  -5.51598807e-002,
     -1.83620215e-001},
   {  1.51448563e+000,   4.69493360e-001,   5.97750105e-002,
      9.31053259e-001},
   {  8.09229439e-001,  -6.09990970e-001,  -3.00166642e+000,
      9.74955512e-001},
   { -5.55180521e-004,   5.36596133e-004,   1.99535725e-315,
      5.38755453e-004}},
  {{ -2.11489302e+000,  -1.29436010e+000,   2.42786358e-316,
     -1.64421722e+000},
   {  6.42696765e-001,   1.40664936e+000,  -5.99178201e-316,
      1.25815930e+000},
   {  8.08252200e-002,   6.09709158e-002,  -1.10647688e-315,
     -1.79345785e+000},
   {  1.55423988e+000,   2.00427408e+000,   5.39064122e-316,
      1.90842736e+000}},
  {{  9.21402245e+000,   9.21402245e+000,   9.21402245e+000,
      9.21402245e+000},
   {  3.25603213e+000,   3.25603213e+000,   3.25603213e+000,
      3.25603213e+000},
   {  5.74711369e-316,   5.74711369e-316,   5.74711369e-316,
      5.74711369e-316},
   {  3.42344902e+000,   3.42344902e+000,   3.42344902e+000,
      3.42344902e+000}}
};

const float bias[depth-1][MaxNode] = {
  {  2.58449936,   2.93605699,  18.8547504 ,   0.45296556},
  {-14.7136522 ,  -0.86268925,  -5.42269236,  16.43386181},
  {  5.27515685,  14.69742834,  -0.72974151,  14.75573753},
  { 13.90512218, 0, 0, 0}
};

MQ135_NNCO2Estimator::adcManager::adcManager(void){}

bool MQ135_NNCO2Estimator::adcManager::pushBuffer(DataBuffer &buffer,uint16_t datum){
  uint32_t PastMillis = millis() - buffer.PrevUpdateTime;
  //100ms経過していなければ無視(100より細かい間隔でデータが格納されるのを防ぐ)
  //##1secだけとくべつ！##
  if (buffer.intervalMillis==100 && PastMillis < buffer.intervalMillis)
    return false;
  //24時間以上経過していたら，オーバーフロー判定
  else if(PastMillis>1000*60*60*24)
    PastMillis = buffer.intervalMillis;

  //ずらす数
  uint8_t bufferSlide = PastMillis / buffer.intervalMillis;
  if(bufferSlide==0){
    buffer.buffer[0] = datum;
    return false;
  }
  //データをupdateすることが確定したので，現時刻を格納
  buffer.PrevUpdateTime = millis();
  //すでに格納されているデータをずらす．欠損したデータは0で埋める
  for(int i = buffer.length-1; i>=0; i--){
    int  sourceIndex = i - bufferSlide;
    if (sourceIndex>=0) buffer.buffer[i] = buffer.buffer[sourceIndex];
    else buffer.buffer[i] = 0;
  }
  //頭に最新の値を入れる
  buffer.buffer[0] = datum;
  return true;
}

float MQ135_NNCO2Estimator::adcManager::bufferAvg(DataBuffer const&buffer, uint32_t msecRange){
  uint32_t sum = 0;
  uint8_t dataCount = 0;
  uint8_t sumRange = msecRange / buffer.intervalMillis;
  for(int i = 0; i<=sumRange-1; i++){
    if(buffer.buffer[i]!=0){
      sum += buffer.buffer[i];
      dataCount++;
    }
  }
  if(dataCount==0) return 0;
  float average = (float)sum / dataCount;
  return average;
}

bool MQ135_NNCO2Estimator::adcManager::update(uint16_t datum){
  if(datum > 1023) datum = 1023;//1024だと16bitから溢れて0になってしまうので
  datum = datum << 6;//6bit分有効活用
  if(pushBuffer(_adc1sec, datum)==false)
    return false;
  pushBuffer(_adc10sec, (uint16_t)bufferAvg(_adc1sec, msec_1sec));
  pushBuffer(_adc1min, (uint16_t)bufferAvg(_adc10sec, msec_10sec));
  pushBuffer(_adc10min, (uint16_t)bufferAvg(_adc1min, msec_1min));
  pushBuffer(_adc1h, (uint16_t)bufferAvg(_adc10min, msec_10min));
  pushBuffer(_adc24h, (uint16_t)bufferAvg(_adc1h, msec_1h));
  return true;
}

float MQ135_NNCO2Estimator::adcManager::secAverage(float timeRangeSec){
  uint32_t msecRange = timeRangeSec * 1000;
  uint16_t avg;
  if(msecRange<100) avg = _adc1sec.buffer[0];
  else if(msecRange<=msec_1sec) avg = bufferAvg(_adc1sec, msecRange);
  else if(msecRange<=msec_10sec) avg = bufferAvg(_adc10sec, msecRange);
  else if(msecRange<=msec_1min) avg = bufferAvg(_adc1min, msecRange);
  else if(msecRange<=msec_10min) avg = bufferAvg(_adc10min, msecRange);
  else if(msecRange<=msec_1h) avg = bufferAvg(_adc1h, msecRange);
  else if(msecRange<=msec_24h) avg = bufferAvg(_adc24h, msecRange);
  //24時間以上の平均を要求されたら，24時間の平均を返す
  else avg = bufferAvg(_adc24h, msec_24h);
  return (float)avg / 64;//6ビットシフト
}

MQ135_NNCO2Estimator::tempManager::tempManager(void){}

bool MQ135_NNCO2Estimator::tempManager::pushBuffer(tempBuffer &buffer,int16_t datum){
  uint32_t PastMillis = millis() - buffer.PrevUpdateTime;
  //100ms経過していなければ無視(100より細かい間隔でデータが格納されるのを防ぐ)
  //##1secだけとくべつ！##
  if (buffer.intervalMillis==100 && PastMillis < buffer.intervalMillis)
    return false;
  //24時間以上経過していたら，オーバーフロー判定
  else if(PastMillis>1000*60*60*24)
    PastMillis = buffer.intervalMillis;

  //ずらす数
  uint8_t bufferSlide = PastMillis / buffer.intervalMillis;
  if(bufferSlide==0){
    buffer.buffer[0] = datum;
    return false;
  }
  //データをupdateすることが確定したので，現時刻を格納
  buffer.PrevUpdateTime = millis();
  //すでに格納されているデータをずらす．欠損したデータは0で埋める
  for(int i = buffer.length-1; i>=0; i--){
    int  sourceIndex = i - bufferSlide;
    if (sourceIndex>=0) buffer.buffer[i] = buffer.buffer[sourceIndex];
    else buffer.buffer[i] = 0;
  }
  //頭に最新の値を入れる
  buffer.buffer[0] = datum;
  return true;
}

float MQ135_NNCO2Estimator::tempManager::bufferAvg(tempBuffer const&buffer, int32_t msecRange){
  int32_t sum = 0;
  uint8_t dataCount = 0;
  uint8_t sumRange = msecRange / buffer.intervalMillis;
  for(int i = 0; i<=sumRange-1; i++){
    if(buffer.buffer[i]!=0){
      sum += buffer.buffer[i];
      dataCount++;
    }
  }
  if(dataCount==0) return 0;
  float average = (float)sum / dataCount;
  return average;
}

bool MQ135_NNCO2Estimator::tempManager::update(float datum){
  datum = datum*100;//uint16に小数を格納するために100倍する
  if(pushBuffer(_temp1sec, datum)==false)
    return false;
  pushBuffer(_temp10sec, (int16_t)bufferAvg(_temp1sec, msec_1sec));
  pushBuffer(_temp1min, (int16_t)bufferAvg(_temp10sec, msec_10sec));
  pushBuffer(_temp10min, (int16_t)bufferAvg(_temp1min, msec_1min));
  pushBuffer(_temp1h, (int16_t)bufferAvg(_temp10min, msec_10min));
  pushBuffer(_temp24h, (int16_t)bufferAvg(_temp1h, msec_1h));
  return true;
}

float MQ135_NNCO2Estimator::tempManager::secAverage(float timeRangeSec){
  uint32_t msecRange = timeRangeSec * 1000;
  uint16_t avg;
  if(msecRange<100) avg = _temp1sec.buffer[0];
  else if(msecRange<=msec_1sec) avg = bufferAvg(_temp1sec, msecRange);
  else if(msecRange<=msec_10sec) avg = bufferAvg(_temp10sec, msecRange);
  else if(msecRange<=msec_1min) avg = bufferAvg(_temp1min, msecRange);
  else if(msecRange<=msec_10min) avg = bufferAvg(_temp10min, msecRange);
  else if(msecRange<=msec_1h) avg = bufferAvg(_temp1h, msecRange);
  else if(msecRange<=msec_24h) avg = bufferAvg(_temp24h, msecRange);
  //24時間以上の平均を要求されたら，24時間の平均を返す
  else avg = bufferAvg(_temp24h, msec_24h);
  return (float)avg / 100;//100倍してあるので戻す
}


MQ135_NNCO2Estimator::MQ135_NNCO2Estimator(){
}

float MQ135_NNCO2Estimator::relu(float inputSum){
  if(inputSum<0) return 0;
  else return inputSum;
}

float MQ135_NNCO2Estimator::getWeight(uint8_t layer, uint8_t node, uint8_t output){
  return w[layer][node][output];
}

float MQ135_NNCO2Estimator::getBias(uint8_t layer, uint8_t node){
  return bias[layer][node];
}

float MQ135_NNCO2Estimator::estimate(void){
  /******ニューラルネットワーク*****/

  //最大ノード数の配列を用意．初期値をセット
  float prevLayerNode[MaxNode] = {adc.secAverage(60),adc.secAverage(60*10),adc.secAverage(60*60),adc.secAverage(60*60*24)};

  //層ごとに計算
  for(int layer=0; layer<=depth-2; layer++){
    float nextLayerNode[MaxNode] = {0};
    //各層のノードごとに計算
    for(int node=0; node<=nodeNum[layer]-1; node++){
      //次層のノード数 = そのノードから出力されるデータ数
      for(int output=0; output<=nodeNum[layer+1]-1; output++){
        //flashに格納されている重みを取得
        float w = getWeight(layer,node,output);
        //層に格納されている値と重みの積を次の層に格納
        nextLayerNode[output] += prevLayerNode[node] * w;
      }
    }
    for(int node=0; node<=nodeNum[layer]-1; node++){
      //バイアス
      nextLayerNode[node] += getBias(layer,node);
      //活性化関数
      nextLayerNode[node] = relu(nextLayerNode[node]);
      //コピー
      prevLayerNode[node] = nextLayerNode[node];
    }
  }
  return prevLayerNode[0];
}
