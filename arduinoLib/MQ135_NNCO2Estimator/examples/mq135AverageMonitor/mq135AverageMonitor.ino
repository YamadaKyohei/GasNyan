#include "MQ135_NNCO2Estimator.h"

//ピン番号
#define greenPin 14
#define redPin 12
#define bluePin 13
#define heaterPin 5
#define adcPin A0
#define dhtPin 4
#define dhtType DHT22

MQ135_NNCO2Estimator mq135(A0);

void setup() {
  pinMode(greenPin, OUTPUT);
  pinMode(redPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  pinMode(heaterPin, OUTPUT);
  pinMode(adcPin, INPUT);
  pinMode(dhtPin, INPUT);
  Serial.begin(115200);
  digitalWrite(heaterPin, HIGH);
}

void loop() {
  if (mq135.update() == true) {
    Serial.println(mq135.avg(0)-mq135.avg(30));
  }
}
