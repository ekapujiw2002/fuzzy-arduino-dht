#include "FuzzyDHT.h"

// detail implementation
// init class
FuzzyDHT::FuzzyDHT() {
  // init first data
  duration_out = 0.0;

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

// begin
void FuzzyDHT::begin(void) {}

/**
 * create new fuzzy rule with and boolean
 * @method createNewFuzzyRule
 * @param  ruleId             id rule
 * @param  in1                fuzzy set 1
 * @param  in2                fuzzy set 2
 * @param  out1               fuzzt set output
 * @return                    new fuzzy rule
 */
FuzzyRule *FuzzyDHT::createNewFuzzyRule(int ruleId, FuzzySet *in1,
                                        FuzzySet *in2, FuzzySet *out1) {
  FuzzyRuleConsequent *fzThen = new FuzzyRuleConsequent();
  fzThen->addOutput(out1);

  FuzzyRuleAntecedent *fzIf = new FuzzyRuleAntecedent();
  fzIf->joinWithAND(in1, in2);

  return new FuzzyRule(ruleId, fzIf, fzThen);
}

/**
 * calculate duration fuzzy
 * @method fuzzyProcessInput
 * @param  tempx             temperature
 * @param  humx              humidity
 * @param  duration_out      output duration
 */
void FuzzyDHT::update(float tempx, float humx) {
  fuzzy_main_obj->setInput(FUZZY_IN_SUHU, tempx);
  fuzzy_main_obj->setInput(FUZZY_IN_HUM, humx);

  fuzzy_main_obj->fuzzify();

  duration_out = fuzzy_main_obj->defuzzify(FUZZY_OUT_SIRAM);
}
