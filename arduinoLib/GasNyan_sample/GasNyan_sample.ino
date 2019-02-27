#include "MQ135_NNCO2Estimator.h"　//自作ニューラルネットワークライブラリ
#include "DHT.h"  //温湿度センサのライブラリ

//ピン番号の定義
#define greenPin 14
#define redPin 12
#define bluePin 13
#define heaterPin 5
#define adcPin A0
#define dhtPin 4
#define dhtType DHT22

DHT dht(dhtPin, dhtType);
MQ135_NNCO2Estimator mq135;

//CO2濃度から発光色を設定する関数
void setLedColor(float co2);

void setup() {
  //IO設定
  pinMode(greenPin, OUTPUT);
  pinMode(redPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  pinMode(heaterPin, OUTPUT);
  pinMode(adcPin, INPUT);
  pinMode(dhtPin, INPUT);
  Serial.begin(115200);
  //ヒーター加熱開始
  digitalWrite(heaterPin, HIGH);
}

void loop() {
  //センサのデータを読み込む
  int adcValue = analogRead(adcPin);  //MQ135のadc値(0-1023)
  float t = dht.readTemperature();    //温度
  float h = dht.readHumidity();       //湿度

  //温湿度センサの読み取りに失敗することがあるので，成功するまで繰り返す
  while (isnan(h) || isnan(t)) {
    Serial.println("DHT error");
    delay(500);
    t = dht.readTemperature();
    h = dht.readHumidity();
  }

  //ニューラルネットワークにセンサのデータを入力する
  mq135.adc.update(adcValue);   //adc
  mq135.temperature.update(t);  //温度
  mq135.humidity.update(h);     //湿度

  //ニューラルネットワークで現在のCO2を推測
  float co2 = mq135.estimate();

  //Serial.println(mq135.adc.secAverage(600));

  //LEDの色を変更
  setLedColor(co2);
  //シリアル出力
  Serial.println(co2);

  delay(1000);
}

void setLedColor(float co2) {
  int _max = 2000; //赤になるときのCO2濃度
  int _min = 500;  //緑になるときのCO2濃度
  
  if (co2 > _max) {
    analogWrite(redPin, 1023);
    analogWrite(greenPin, 0);
  }
  else if (co2 < _min) {
    analogWrite(redPin, 0);
    analogWrite(greenPin, 1023);
  }
  else {
    int a = (int)((double)(co2 - _min) / (double)(_max - _min) * 1023);
    int b = (int)((double)(_max - co2) / (double)(_max - _min) * 1023);
    analogWrite(redPin, a);
    analogWrite(greenPin, b);
  }
}

