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
#include <Fuzzy.h>
#include <FuzzyComposition.h>
#include <FuzzyIO.h>
#include <FuzzyInput.h>
#include <FuzzyOutput.h>
#include <FuzzyRule.h>
#include <FuzzyRuleAntecedent.h>
#include <FuzzyRuleConsequent.h>
#include <FuzzySet.h>

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

// fuzzy id
#define FUZZY_IN_SUHU 1
#define FUZZY_IN_HUM 2
#define FUZZY_OUT_SIRAM 3

// fuzzy object
Fuzzy *fuzzy_main_obj = new Fuzzy();

// input suhu
FuzzySet *suhu_dingin = new FuzzySet(0, 0, 19, 25);
FuzzySet *suhu_normal = new FuzzySet(20, 25, 25, 30);
FuzzySet *suhu_panas = new FuzzySet(25, 30, 50, 50);

// input humidity
FuzzySet *hum_kering = new FuzzySet(0, 0, 50, 70);
FuzzySet *hum_normal = new FuzzySet(50, 70, 70, 90);
FuzzySet *hum_lembab = new FuzzySet(70, 90, 100, 100);

// output durasi siram
FuzzySet *siram_sebentar = new FuzzySet(0, 0, 7, 10);
FuzzySet *siram_cukup = new FuzzySet(7, 10, 10, 12);
FuzzySet *siram_lama = new FuzzySet(10, 12, 15, 15);

float fuzzy_duration_out = 0.0;

// lc dobj
LiquidCrystal lcd_obj(12, 11, 4, 5, 6, 7); // Creates an LC object. Parameters:
                                           // (rs, enable, d4, d5, d6, d7) , rw
                                           // to ground

// timing var
uint32_t t_now, t_last_dht_acquired, t_last_fuzzy, t_last_lcd_display;
uint32_t t_relay_start_on = 0;

void APP_DEBUG_PRINT(String alog) {
  char dtx[16] = {0};
  snprintf_P(dtx, sizeof(dtx), (const char *)F("%-10u : "), millis());
  APP_PORT_DEBUG.println(String(dtx) + alog);
}

/**
 * create new fuzzy rule with and boolean
 * @method createNewFuzzyRule
 * @param  ruleId             id rule
 * @param  in1                fuzzy set 1
 * @param  in2                fuzzy set 2
 * @param  out1               fuzzt set output
 * @return                    new fuzzy rule
 */
FuzzyRule *createNewFuzzyRule(int ruleId, FuzzySet *in1, FuzzySet *in2,
                              FuzzySet *out1) {
  FuzzyRuleConsequent *fzThen = new FuzzyRuleConsequent();
  fzThen->addOutput(out1);

  FuzzyRuleAntecedent *fzIf = new FuzzyRuleAntecedent();
  fzIf->joinWithAND(in1, in2);

  return new FuzzyRule(ruleId, fzIf, fzThen);
}

/**
 * init all fuzzy input, rule, and output
 * @method fuzzyInit
 */
void fuzzyInit() {
  // FuzzyInput suhu
  FuzzyInput *fz_suhu = new FuzzyInput(FUZZY_IN_SUHU);
  fz_suhu->addFuzzySet(suhu_dingin);
  fz_suhu->addFuzzySet(suhu_normal);
  fz_suhu->addFuzzySet(suhu_panas);
  fuzzy_main_obj->addFuzzyInput(fz_suhu);

  // FuzzyInput hum
  FuzzyInput *fz_hum = new FuzzyInput(FUZZY_IN_HUM);
  fz_hum->addFuzzySet(hum_kering);
  fz_hum->addFuzzySet(hum_normal);
  fz_hum->addFuzzySet(hum_lembab);
  fuzzy_main_obj->addFuzzyInput(fz_hum);

  // FuzzyOutput siram
  FuzzyOutput *fz_siram = new FuzzyOutput(FUZZY_OUT_SIRAM);
  fz_siram->addFuzzySet(siram_sebentar);
  fz_siram->addFuzzySet(siram_cukup);
  fz_siram->addFuzzySet(siram_lama);
  fuzzy_main_obj->addFuzzyOutput(fz_siram);

  // fuzzy rule
  fuzzy_main_obj->addFuzzyRule(
      createNewFuzzyRule(1, suhu_dingin, hum_kering, siram_sebentar));

  fuzzy_main_obj->addFuzzyRule(
      createNewFuzzyRule(2, suhu_dingin, hum_normal, siram_cukup));

  fuzzy_main_obj->addFuzzyRule(
      createNewFuzzyRule(3, suhu_dingin, hum_lembab, siram_sebentar));

  fuzzy_main_obj->addFuzzyRule(
      createNewFuzzyRule(4, suhu_normal, hum_kering, siram_sebentar));

  fuzzy_main_obj->addFuzzyRule(
      createNewFuzzyRule(5, suhu_normal, hum_normal, siram_cukup));

  fuzzy_main_obj->addFuzzyRule(
      createNewFuzzyRule(6, suhu_normal, hum_lembab, siram_cukup));

  fuzzy_main_obj->addFuzzyRule(
      createNewFuzzyRule(7, suhu_panas, hum_kering, siram_lama));

  fuzzy_main_obj->addFuzzyRule(
      createNewFuzzyRule(8, suhu_panas, hum_normal, siram_cukup));

  fuzzy_main_obj->addFuzzyRule(
      createNewFuzzyRule(9, suhu_panas, hum_lembab, siram_lama));
}

/**
 * calculate duration fuzzy
 * @method fuzzyProcessInput
 * @param  tempx             temperature
 * @param  humx              humidity
 * @param  duration_out      output duration
 */
void fuzzyProcessInput(float tempx, float humx, float *duration_out) {
  fuzzy_main_obj->setInput(FUZZY_IN_SUHU, tempx);
  fuzzy_main_obj->setInput(FUZZY_IN_HUM, humx);

  fuzzy_main_obj->fuzzify();

  *duration_out = fuzzy_main_obj->defuzzify(FUZZY_OUT_SIRAM);
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
          fuzzyProcessInput(dht_sensor_output.temperature,
                            dht_sensor_output.humidity, &fuzzy_duration_out);
          t_relay_start_on = 0;

          APP_DEBUG_PRINT(String("DURATION = ") + String(fuzzy_duration_out));
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
  if (fuzzy_duration_out > 0.0) {
    if (t_relay_start_on == 0) {
      t_relay_start_on = millis();
    }

    digitalWrite(PIN_RELAY, ((millis() - t_relay_start_on) <=
                             ((uint32_t)(fuzzy_duration_out * 1000.0))));
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
                   fuzzy_duration_out);
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
  float durx = 0.0;

  // set timeout for serial
  APP_PORT_DEBUG.setTimeout(30000);

  APP_DEBUG_PRINT(F("TEMP = "));
  float tempx = APP_PORT_DEBUG.readStringUntil('\r').toFloat();
  APP_DEBUG_PRINT(F("HUM = "));
  float humx = APP_PORT_DEBUG.readStringUntil('\r').toFloat();

  if ((tempx >= 0.0) && (humx >= 0.00)) {
    fuzzyProcessInput(tempx, humx, &durx);

    APP_DEBUG_PRINT(String("TEMPFZ = ") + String(tempx) + String(" -- ") +
                    String(suhu_dingin->getPertinence()) + String(" -- ") +
                    String(suhu_normal->getPertinence()) + String(" -- ") +
                    String(suhu_panas->getPertinence()));

    APP_DEBUG_PRINT(String("HUMFZ = ") + String(humx) + String(" -- ") +
                    String(hum_kering->getPertinence()) + String(" -- ") +
                    String(hum_normal->getPertinence()) + String(" -- ") +
                    String(hum_lembab->getPertinence()));

    APP_DEBUG_PRINT(String("DURATION = ") +
                    String((uint32_t)((float)durx * 1000.0)));
  } else {
    APP_DEBUG_PRINT(F("INVALID VALUE"));
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

/**
 * all setup goes here
 * @method setup
 */
void setup() {
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
  fuzzyInit();

  APP_DEBUG_PRINT(F("INIT DONE"));

  for (;;) {
    debugTest();
  }
}

/**
 * main loop
 * @method loop
 */
void loop() {
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
