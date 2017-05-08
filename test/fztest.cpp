// main lib
#include <Arduino.h>

#include <FuzzyDHT.h>
#include <dht_util.h>

// pin setting
#define PIN_DHT 8
#define PIN_RELAY 9

// global var
// rtc
// DS3231 rtc(SDA, SCL);

// dht sensor
DHT dht_sensor(PIN_DHT, DHT22);
dht_data_t dht_sensor_output = {0};

FuzzyDHT *myfuzzy = new FuzzyDHT();

void setup() {
  // dht up
  dht_sensor.begin();

  myfuzzy->begin();
}

void loop() {
  get_dht_data(&dht_sensor, &dht_sensor_output);
  myfuzzy->update(dht_sensor_output.temperature, dht_sensor_output.humidity);

  delay(5000);
}
