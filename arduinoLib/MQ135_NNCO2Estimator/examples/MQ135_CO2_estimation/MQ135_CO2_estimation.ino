#include "MQ135_NNCO2Estimator.h"

//ピン番号
#define greenPin 14
#define redPin 12
#define bluePin 13
#define heaterPin 5
#define adcPin A0

MQ135_NNCO2Estimator mq135;

void setup() {
  pinMode(greenPin, OUTPUT);
  pinMode(redPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  pinMode(heaterPin, OUTPUT);
  pinMode(adcPin, INPUT);
  //pinMode(dhtPin, INPUT);
  Serial.begin(115200);
  digitalWrite(heaterPin, HIGH);
}

void loop() {
  delay(100);

  int adcValue = analogRead(adcPin);
  float tempValue = 50.999;

  mq135.adc.update(adcValue);
  mq135.temperature.update(tempValue);
  //mq135.humidity.update();
  float co2 = mq135.estimate();
  Serial.println(mq135.adc.secAverage(600));
}
