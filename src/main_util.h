#ifndef MAIN_UTIL_H
#define MAIN_UTIL_H

// main lib
#include <Arduino.h>

// lcd lib
#include <LiquidCrystal.h>

// dht sensor
#include "dht_util.h"
#include <DHT.h>

// rtc
#include <DS3231.h>

// fuzzy
#include <FuzzyDHT.h>

// debug port
#define APP_PORT_DEBUG Serial

// pin setting
#define PIN_DHT 8
#define PIN_RELAY 9

// global var
// rtc
DS3231 rtc(SDA, SCL);

// dht sensor
DHT dht_sensor(PIN_DHT, DHT22);
dht_data_t dht_sensor_output = {0};

// fuzzy object
FuzzyDHT *fuzzy_main_obj = new FuzzyDHT();

// lc dobj
LiquidCrystal lcd_obj(12, 11, 4, 5, 6, 7); // Creates an LC object. Parameters:
                                           // (rs, enable, d4, d5, d6, d7) , rw
                                           // to ground

// timing var
uint32_t t_now, t_last_dht_acquired, t_last_fuzzy, t_last_lcd_display;
uint32_t t_relay_start_on = 0;

/**
 * debug printing util
 * @method APP_DEBUG_PRINT
 * @param  alog            [description]
 */
void APP_DEBUG_PRINT(String alog) {
  char dtx[16] = {0};
  snprintf_P(dtx, sizeof(dtx), (const char *)F("%-10u : "), millis());
  APP_PORT_DEBUG.println(String(dtx) + alog);
}

/**
 * init lcd
 * @method lcd_init
 */
void lcd_init() {
  lcd_obj.begin(16, 2);
  lcd_obj.clear();
  lcd_obj.noCursor();
  // lcd_obj.home();
  lcd_obj.setCursor(3, 0);
  lcd_obj.print("FUZZY DHT");
  lcd_obj.setCursor(5, 1);
  lcd_obj.print("2017");
  delay(2000);
}

/**
 * show data to lcd
 * @method lcd_print_data
 * @param  lcdx           lcd object
 * @param  atime          the datetime
 * @param  dht_data_out   dht data
 * @param  dur_on         duration relay on
 */
void lcd_print_data(LiquidCrystal *lcdx, Time atime, dht_data_t dht_data_out,
                    float dur_on) {
  // show datetime
  char dtx[18] = {0};
  snprintf_P(dtx, sizeof(dtx), (const char *)F("%02d-%02d-%04d %02d%c%02d"),
             atime.date, atime.mon, atime.year, atime.hour,
             (atime.sec % 2) ? ' ' : ':', atime.min);
  lcdx->setCursor(0, 0);
  lcdx->print(dtx);

  // show dht value
  memset(dtx, ' ', sizeof(dtx));
  dtostrf(dht_data_out.status_ok ? dht_data_out.temperature : -1.0, 4, 1,
          &dtx[0]);
  dtx[4] = ' ';
  dtostrf(dht_data_out.status_ok ? dht_data_out.humidity : -1.0, 4, 1, &dtx[5]);
  dtx[9] = ' ';
  dtostrf(dur_on, 5, 1, &dtx[10]);

  lcdx->setCursor(0, 1);
  lcdx->print(dtx);
}

/**
 * processing fuzzy way
 * @method processFuzzySystem
 */
void processFuzzySystem() {
  Time tx = rtc.getTime();

  // 7 to 15, per 2 hours
  if ((tx.hour >= 7) && (tx.hour <= 15) && (tx.hour % 2 == 1)) {
    if (tx.min <= 0) {
      if (tx.sec <= (5 + (dht_sensor_output.status_ok * 5))) {
        // process it
        // process input if is valid
        if (dht_sensor_output.status_ok) {
          fuzzy_main_obj->update(dht_sensor_output.temperature,
                                 dht_sensor_output.humidity);

          t_relay_start_on = 0;

          APP_DEBUG_PRINT(String("DURATION = ") +
                          String(fuzzy_main_obj->duration_out));
        }
      }
    }
  }
}

/**
 * relay on or off
 * @method processRelayOnOff
 */
void processRelayOnOff() {
  if (fuzzy_main_obj->duration_out > 0.0) {
    if (t_relay_start_on == 0) {
      t_relay_start_on = millis();
    }

    digitalWrite(PIN_RELAY,
                 ((millis() - t_relay_start_on) <=
                  ((uint32_t)(fuzzy_main_obj->duration_out * 1000.0))));
  } else {
    if (t_relay_start_on != 0) {
      t_relay_start_on = 0;
    }
    digitalWrite(PIN_RELAY, LOW);
  }
}

/**
 * display data to lcd
 * @method processLCDDisplayData
 */
void processLCDDisplayData() {
  if (t_now - t_last_lcd_display >= 1000) {
    t_last_lcd_display = t_now;
    lcd_print_data(&lcd_obj, rtc.getTime(), dht_sensor_output,
                   fuzzy_main_obj->duration_out);
  }
}

/**
 * get dht data sensor
 * @method processDHTSensor
 */
void processDHTSensor() {
  if (t_now - t_last_dht_acquired >= 5000) {
    t_last_dht_acquired = t_now;
    get_dht_data(&dht_sensor, &dht_sensor_output);

    if (!dht_sensor_output.status_ok) {
      APP_DEBUG_PRINT(F("DHT ERROR"));
    }
  }
}

/**
 * debug fuzzy system
 * @method debugTest
 */
void debugTest() {

  // enter debug mode or not
  APP_DEBUG_PRINT(F("ENTER DEBUG MODE [Y/N]?"));
  String respx = APP_PORT_DEBUG.readStringUntil('\r');

  if (respx.equalsIgnoreCase("y")) {
    // set timeout for serial
    APP_PORT_DEBUG.setTimeout(30000);

    while (1) {
      APP_DEBUG_PRINT(F("TEMP = "));
      float tempx = APP_PORT_DEBUG.readStringUntil('\r').toFloat();
      APP_DEBUG_PRINT(F("HUM = "));
      float humx = APP_PORT_DEBUG.readStringUntil('\r').toFloat();

      if ((tempx >= 0.0) && (humx >= 0.00)) {
        fuzzy_main_obj->update(tempx, humx);

        // fuzzyProcessInput(tempx, humx, &durx);
        //
        // APP_DEBUG_PRINT(String("TEMPFZ = ") + String(tempx) + String(" -- ")
        // +
        //                 String(suhu_dingin->getPertinence()) + String(" -- ")
        //                 +
        //                 String(suhu_normal->getPertinence()) + String(" -- ")
        //                 +
        //                 String(suhu_panas->getPertinence()));
        //
        // APP_DEBUG_PRINT(String("HUMFZ = ") + String(humx) + String(" -- ") +
        //                 String(hum_kering->getPertinence()) + String(" -- ")
        //                 +
        //                 String(hum_normal->getPertinence()) + String(" -- ")
        //                 +
        //                 String(hum_lembab->getPertinence()));

        APP_DEBUG_PRINT(
            String("DURATION = ") +
            String((uint32_t)((float)fuzzy_main_obj->duration_out * 1000.0)));
      } else {
        APP_DEBUG_PRINT(F("INVALID VALUE"));
      }
    }
  }
}

/**
 * init relay gpio
 * @method relayInit
 */
void relayInit() {
  pinMode(PIN_RELAY, OUTPUT);
  digitalWrite(PIN_RELAY, LOW);
}

void main_app_setup() {
  APP_PORT_DEBUG.begin(19200);
  APP_DEBUG_PRINT(F("FUZZIFIER DHT22 INITIALIZE"));

  // init relay
  relayInit();

  // lcd up
  lcd_init();

  // rtc up
  rtc.begin();

  // dht up
  dht_sensor.begin();

  // init timing
  t_now = t_last_dht_acquired = t_last_fuzzy = t_last_lcd_display = millis();

  // init fuzzy
  fuzzy_main_obj->begin();

  APP_DEBUG_PRINT(F("INIT DONE"));

  debugTest();
}

void main_app_loop() {
  // current time
  t_now = millis();

  // display to lcd per 1sec
  processLCDDisplayData();

  // dht last time
  processDHTSensor();

  // relay on or off
  processRelayOnOff();

  // time to process
  processFuzzySystem();

  // fuzzy last time
  // if (t_now - t_last_fuzzy >= 5000) {
  //   t_last_fuzzy = t_now;
  //
  //   // process input if is valid
  //   if (dht_sensor_output.status_ok) {
  //     fuzzyProcessInput(dht_sensor_output.temperature,
  //                       dht_sensor_output.humidity, &fuzzy_duration_out);
  //
  //     APP_DEBUG_PRINT(String(fuzzy_duration_out));
  //   }
  // }
}
#endif
