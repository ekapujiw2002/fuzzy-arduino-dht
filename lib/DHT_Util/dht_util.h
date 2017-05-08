#ifndef _DHT_UTIL_H_
#define _DHT_UTIL_H_

// dht library
#include <DHT.h>

// dht22 data sensor struct
typedef struct {
  uint8_t status_ok;
  float humidity, temperature;
} dht_data_t;

// proto void func
uint8_t get_dht_data(DHT *dht_obj, dht_data_t *dht_data_output);

/**
 * Get humidity and temperature from dht sensor object
 * @method get_dht_data
 * @param  dht_obj         DHT sensor object
 * @param  dht_data_output dht data output
 * @return                 0=error, 1=ok
 */
uint8_t get_dht_data(DHT *dht_obj, dht_data_t *dht_data_output) {
  float hum = dht_obj->readHumidity();
  // Read temperature as Celsius (the default)
  float tempx = dht_obj->readTemperature();

  // Check if any reads failed and exit early (to try again).
  uint8_t sts_ok = (!isnan(hum) && !isnan(tempx));

  if (sts_ok) {
    dht_data_output->humidity = hum;
    dht_data_output->temperature = tempx;
    dht_data_output->status_ok = 1;
  }
  return sts_ok;
}
#endif
