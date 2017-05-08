#ifndef FUZZYDHT_H
#define FUZZYDHT_H

#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WConstants.h"
#include "WProgram.h"
#include "pins_arduino.h"
#endif

// fuzzy lib
#include <Fuzzy.h>
#include <FuzzyComposition.h>
#include <FuzzyIO.h>
#include <FuzzyInput.h>
#include <FuzzyOutput.h>
#include <FuzzyRule.h>
#include <FuzzyRuleAntecedent.h>
#include <FuzzyRuleConsequent.h>
#include <FuzzySet.h>

// fuzzy id
#define FUZZY_IN_SUHU 1
#define FUZZY_IN_HUM 2
#define FUZZY_OUT_SIRAM 3

// class fuzzy from dht
class FuzzyDHT {
public:
  FuzzyDHT();
  void begin(void);

  float duration_out;

  void update(float tempx, float humx);

private:
  // main fuzzy object
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

  FuzzyRule *createNewFuzzyRule(int ruleId, FuzzySet *in1, FuzzySet *in2,
                                FuzzySet *out1);
};

#endif
