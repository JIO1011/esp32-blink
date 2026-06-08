#include <Arduino.h>

#define LED_PIN 2

void setup() {
  // put your setup code here, to run once:
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(115200);
  Serial.println("Hello, ESP32!");
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(LED_PIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  Serial.println("LED is ON");
  delay(1000);                  // wait for a second
  digitalWrite(LED_PIN, LOW);    // turn the LED off by making the voltage LOW
  Serial.println("LED is OFF");
  delay(1000);                  // wait for a second


}