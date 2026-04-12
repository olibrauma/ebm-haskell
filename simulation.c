#include "ebm.h"

void dump_state(const char *dir, const char *filename, int loop, double season,
                double P_air, double P_ice, double P_rego, double T_sub,
                double T[], double M[], double T_posi[], double M_posi[],
                double T_nega[], double M_nega[]) {
  char path[256];
  sprintf(path, "%s/%s", dir, filename);
  FILE *f = fopen(path, "w");
  if (!f)
    return;
  fprintf(f, "loop:%d,season:%.16g,P_air:%.16g,P_ice:%.16g,P_rego:%.16g,T_sub:%.16g\n",
          loop, season, P_air, P_ice, P_rego, T_sub);
  for (int i = 0; i < 181; i++) {
    fprintf(f, "%d,%.16g,%.16g,%.16g,%.16g,%.16g,%.16g\n", i, T[i], M[i],
            T_posi[i], M_posi[i], T_nega[i], M_nega[i]);
  }
  fclose(f);
}

void heikou(const SimulationConfig *conf, const PlanetParams *para,
            ClimateState *state) {
  int reso = conf->reso;
  int n, loop = 0;
  double T_sub, abs = 0.0, abs_posi = 0.0, abs_nega = 0.0, loop_end = 0.0;
  double ins[reso], dif[reso], radi[reso];
  double ins_posi[reso], radi_posi[reso], ins_nega[reso], radi_nega[reso],
      dif_posi[reso], dif_nega[reso];
  double T_last[reso], T_last_posi[reso], T_last_nega[reso];
  double local_season =
      state->season; // Use local season logic based on Phase 1 fix

  // Initialize loop limit tracking matching Origin
  // Origin heikou loops roughly 1 Martian Year.
  // Our Phase 1 verification showed "heikou runs for exactly 1 Martian year"

  for (n = 0; n < reso; n++) {
    T_last[n] = state->T[n];
    T_last_posi[n] = state->T_posi[n];
    T_last_nega[n] = state->T_nega[n];
  }

  do {
    abs = 0.0;
    abs_posi = 0.0;
    abs_nega = 0.0;
    fprintf(stderr, "The %d th loop begins.\n", loop);
    do {
      // Step 1: Global
      calc_ins(conf, para, state, ins);
      calc_dif(conf, para, state, dif);
      calc_radi(conf, para, state, radi);
      calc_T_M(conf, para, state, ins, dif, radi);
      calc_ice(conf, para, state);
      calc_rego(conf, para, state);

      // Step 2: North Slope
      calc_ins_s(conf, para, state, para->alpha_posi, ins_posi);
      calc_radi_s(conf, para, state, state->T_posi, radi_posi);
      calc_dif_s(conf, para, state, state->T_posi, dif_posi);
      calc_Ts_Ms(conf, para, state, state->T_posi, state->M_posi, ins_posi,
                 radi_posi, dif_posi);

      // Step 3: South Slope
      calc_ins_s(conf, para, state, para->alpha_nega, ins_nega);
      calc_radi_s(conf, para, state, state->T_nega, radi_nega);
      calc_dif_s(conf, para, state, state->T_nega,
                 dif_nega); // Fix: use T_nega (Phase 1 correction)
      calc_Ts_Ms(conf, para, state, state->T_nega, state->M_nega, ins_nega,
                 radi_nega, dif_nega);

      // We need to manage season carefully.
      // In Refactored test.c: state->season was updated.
      // BUT we used a local variable logic in previous steps?
      // Wait, in Phase 1 Final, `heikou` USES `state->season` directly but
      // resets it? Or does it use a local variable? Let's check test.c view
      // again. Looking at `heikou` in `test.c` (viewed earlier): "Reverted
      // heikou function to use state->season directly and removed the
      // state->season = 1850 reset" Ah, so it advances state->season? Let's
      // stick to what worked in `test.c`. I need to be careful. The `heikou`
      // function signature in `test.c` took `state`. The logic inside `heikou`
      // in `test.c` (Phase 1 Final):
      //   state->season += conf->dt;

      state->season += conf->dt;

    } while (state->season < conf->Year_sec * (loop + 1) && state->bug == 0.0);

    for (n = 0; n < reso; n++) {
      abs += pow(T_last[n] - state->T[n], 2.0);
      T_last[n] = state->T[n];
      abs_posi += pow(T_last_posi[n] - state->T_posi[n], 2.0);
      T_last_posi[n] = state->T_posi[n];
      abs_nega += pow(T_last_nega[n] - state->T_nega[n], 2.0);
      T_last_nega[n] = state->T_nega[n];
    }
    abs = sqrt(abs);
    abs_posi = sqrt(abs_posi);
    abs_nega = sqrt(abs_nega);
    loop += 1;
    if (abs < 1.0 && abs_posi < 1.0 && abs_nega < 1.0)
      loop_end = 1.0;
  } while (loop_end == 0.0 && loop < 1 && state->bug == 0.0);

  // No season reset here based on Phase 1 Final "reverted reset" logic.
  state->loop = loop;
}
