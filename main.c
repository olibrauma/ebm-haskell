#include "ebm.h"

int main(int argc, char *argv[]) {
  SimulationConfig conf;
  PlanetParams para;
  ClimateState state;

  conf.reso = 181;
  conf.dt = 185.0;
  conf.day_sec = 60.0 * (60.0 * 24.0 + 40.0);
  calc_Yearsec(&conf.Year_sec);
  conf.step_limit = 10;

  para.Q = 1366.0;
  para.obl = 25.19 * M_PI / 180.0;
  para.P_total = 0.130;
  para.alpha_posi = 30.0 * M_PI / 180.0;
  para.alpha_nega = -1.0 * para.alpha_posi;

  int n;
  for (n = 0; n < conf.reso; n++) {
    state.T[n] = 250.0;
    state.M[n] = 0.0;
    state.T_posi[n] = state.T[n];
    state.M_posi[n] = 0.0;
    state.T_nega[n] = state.T[n];
    state.M_nega[n] = 0.0;
  }
  state.P_air = para.P_total;
  state.P_ice = 0.0;
  state.P_rego = 0.0;
  state.season = 0.0;
  state.T_sub = 0.0;
  state.bug = 0.0;
  state.loop = 0;

  if (argc > 3) {
    para.P_total = atof(argv[1]);
    para.obl = atof(argv[2]) * M_PI / 180.0;
    para.alpha_posi = atof(argv[3]) * M_PI / 180.0;
    para.alpha_nega = -1.0 * para.alpha_posi;
    state.P_air = para.P_total;
  }
  const char *out_dir = (argc > 4) ? argv[4] : ".";

  heikou(&conf, &para, &state);

  dump_state(out_dir, "dump_000.dat", state.loop, state.season, state.P_air,
             state.P_ice, state.P_rego, state.T_sub, state.T, state.M,
             state.T_posi, state.M_posi, state.T_nega, state.M_nega);

  if (state.bug == 0.0) {
    int step_count = 0;
    do {
      double ins[181], dif[181], radi[181];
      double ins_posi[181], radi_posi[181], ins_nega[181], radi_nega[181],
          dif_posi[181], dif_nega[181];

      calc_ins(&conf, &para, &state, ins);
      calc_dif(&conf, &para, &state, dif);
      calc_radi(&conf, &para, &state, radi);
      calc_T_M(&conf, &para, &state, ins, dif, radi);
      calc_ice(&conf, &para, &state);
      calc_rego(&conf, &para, &state);

      calc_ins_s(&conf, &para, &state, para.alpha_posi, ins_posi);
      calc_radi_s(&conf, &para, &state, state.T_posi, radi_posi);
      calc_dif_s(&conf, &para, &state, state.T_posi, dif_posi);
      calc_Ts_Ms(&conf, &para, &state, state.T_posi, state.M_posi, ins_posi,
                 radi_posi, dif_posi);

      calc_ins_s(&conf, &para, &state, para.alpha_nega, ins_nega);
      calc_radi_s(&conf, &para, &state, state.T_nega, radi_nega);
      calc_dif_s(&conf, &para, &state, state.T_nega, dif_nega);
      calc_Ts_Ms(&conf, &para, &state, state.T_nega, state.M_nega, ins_nega,
                 radi_nega, dif_nega);

      state.season += conf.dt;

      char fname[64];
      sprintf(fname, "dump_%03d.dat", ++step_count);
      dump_state(out_dir, fname, state.loop, state.season, state.P_air,
                 state.P_ice, state.P_rego, state.T_sub, state.T, state.M,
                 state.T_posi, state.M_posi, state.T_nega, state.M_nega);

      if (step_count >= conf.step_limit)
        break;

    } while (state.season < conf.Year_sec * (state.loop + 1));
  }
  return 0;
}
