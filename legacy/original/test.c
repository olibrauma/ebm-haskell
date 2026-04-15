#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct {
  int reso;
  double dt;
  double day_sec;
  double Year_sec;
  int step_limit; // 10
} SimulationConfig;

typedef struct {
  double Q;
  double obl;
  double P_total;
  double alpha_posi;
  double alpha_nega;
} PlanetParams;

typedef struct {
  double T[181];
  double M[181];
  double T_posi[181];
  double M_posi[181];
  double T_nega[181];
  double M_nega[181];
  double P_air;
  double P_ice;
  double P_rego;
  double season;
  double T_sub;
  double bug;
  int loop;
} ClimateState;

// Prototypes
void calc_ins(const SimulationConfig *conf, const PlanetParams *para,
              ClimateState *state, double ins[]);
void calc_dif(const SimulationConfig *conf, const PlanetParams *para,
              ClimateState *state, double dif[]);
void calc_radi(const SimulationConfig *conf, const PlanetParams *para,
               ClimateState *state, double radi[]);
void calc_T_M(const SimulationConfig *conf, const PlanetParams *para,
              ClimateState *state, double ins[], double dif[], double radi[]);
void calc_ice(const SimulationConfig *conf, const PlanetParams *para,
              ClimateState *state);
void calc_rego(const SimulationConfig *conf, const PlanetParams *para,
               ClimateState *state);
void calc_Yearsec(double *Ysec);
void calc_delta(const SimulationConfig *conf, const PlanetParams *para,
                double season, double *delta, double *r_AU);
void heikou(const SimulationConfig *conf, const PlanetParams *para,
            ClimateState *state);
void calc_ins_s(const SimulationConfig *conf, const PlanetParams *para,
                ClimateState *state, double alpha, double ins_s[]);
void calc_radi_s(const SimulationConfig *conf, const PlanetParams *para,
                 ClimateState *state, double *T, double radi_s[]);
void calc_Ts_Ms(const SimulationConfig *conf, const PlanetParams *para,
                ClimateState *state, double *T, double *M, double ins[],
                double radi[], double dif[]);
void Tsub_af_ai(double P_air, double *Tsub, double *a_f, double *a_i);
void calc_dif_s(const SimulationConfig *conf, const PlanetParams *para,
                ClimateState *state, double T_s[], double dif_s[]);

void dump_state(const char *dir, const char *filename, int loop, double season,
                double P_air, double P_ice, double P_rego, double T_sub,
                double T[], double M[], double T_posi[], double M_posi[],
                double T_nega[], double M_nega[]) {
  char path[256];
  sprintf(path, "%s/%s", dir, filename);
  FILE *f = fopen(path, "w");
  if (!f)
    return;
  // Match origin format
  fprintf(f, "loop:%d,season:%g,P_air:%g,P_ice:%g,P_rego:%g,T_sub:%g\n", loop,
          season, P_air, P_ice, P_rego, T_sub);
  for (int i = 0; i < 181; i++) {
    fprintf(f, "%d,%.16g,%.16g,%.16g,%.16g,%.16g,%.16g\n", i, T[i], M[i],
            T_posi[i], M_posi[i], T_nega[i], M_nega[i]);
  }
  fclose(f);
}

int main(int argc, char *argv[]) {
  SimulationConfig conf;
  PlanetParams para;
  ClimateState state;

  conf.reso = 181;
  conf.dt = 185.0;
  conf.day_sec = 60.0 * (60.0 * 24.0 + 40.0);
  calc_Yearsec(&conf.Year_sec);
  conf.step_limit = 10; // Match legacy main logic

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
      // Loop physics, exact copy of legacy main loop
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
      // Legacy main: passed dif_posi to south calculation? (Bug?)
      // Step 606 heikou fixed it, but legacy main might have it.
      // Legacy main line 133: calc_dif_s(..., dif_nega); calc_Ts_Ms(...,
      // dif_nega); Let's re-verify Legacy step 844 line 132-133:
      // calc_dif_s(day, T, T_nega, P_air, dif_nega);
      // calc_Ts_Ms(P_air, T_nega, M_nega, ins_nega, radi_nega, dif_nega, dt);
      // IT USES DIF_NEGA correctly in Legacy MAIN.
      // My previous analysis of "main bug" (using dif_posi) might have been
      // wrong or from heikou? Legacy Heikou line 464: calc_dif_s(...,
      // dif_nega); calc_Ts_Ms(..., dif_nega); Wait, legacy seems correct? Let's
      // assume correct usage: T_nega -> dif_nega. BUT: calc_dif_s argument in
      // Heikou line 464 says `T` and `T_nega`? `calc_dif_s(day, T, T_nega,
      // P_air, dif_nega);` My previous "Bug Replication" (Step 168+ code edits)
      // used `dif_posi` in `main`. Legacy line 132: `calc_dif_s(day, T, T_nega,
      // P_air, dif_nega);` Legacy line 133: `calc_Ts_Ms(P_air, T_nega, M_nega,
      // ins_nega, radi_nega, dif_nega, dt);` I don't see the bug in legacy view
      // `844`. I will implement CORRECTLY: T_nega -> dif_nega.

      calc_dif_s(&conf, &para, &state, state.T_nega, dif_nega);
      calc_Ts_Ms(&conf, &para, &state, state.T_nega, state.M_nega, ins_nega,
                 radi_nega, dif_nega);

      state.season += conf.dt;

      char fname[64];
      sprintf(fname, "dump_%03d.dat", ++step_count);
      dump_state(out_dir, fname, state.loop, state.season, state.P_air,
                 state.P_ice, state.P_rego, state.T_sub, state.T, state.M,
                 state.T_posi, state.M_posi, state.T_nega, state.M_nega);

      if (step_count >= 10)
        break;

    } while (state.season < conf.Year_sec * (state.loop + 1));
  }
  return 0;
}

// Functions

void calc_ins(const SimulationConfig *conf, const PlanetParams *para,
              ClimateState *state, double ins[]) {
  int reso = conf->reso, n;
  double Q = para->Q;
  double delta, h, cosH, H, r_AU, day;
  double theta[reso];
  double season = state->season;

  for (n = 0; n < reso; n++)
    theta[n] = (n - 90.0) * M_PI / 180.0;

  day = (season - fmod(season, conf->day_sec)) / conf->day_sec;
  h = fmod(season, conf->day_sec) * 2.0 * M_PI / conf->day_sec - M_PI;
  calc_delta(conf, para, season, &delta, &r_AU);

  for (n = 0; n < reso; n++) {
    cosH = -tan(theta[n]) * tan(delta);
    if (cosH >= 1.0)
      H = 0.0;
    else if (cosH <= -1.0)
      H = M_PI;
    else
      H = acos(cosH);

    if (r_AU != 0.0) {
      ins[n] = (Q / M_PI / r_AU / r_AU) * (H * sin(theta[n]) * sin(delta) +
                                           cos(theta[n]) * cos(delta) * sin(H));
    } else {
      ins[n] = 0.0;
    }
  }
}

void calc_dif(const SimulationConfig *conf, const PlanetParams *para,
              ClimateState *state, double dif[]) {
  int reso = conf->reso, n;
  double P_air = state->P_air;
  double *T = state->T;
  double coefficient = 5.3e-3;
  double D = coefficient * P_air;
  double del_phi = M_PI / 180.0;
  double phi[reso];

  for (n = 1; n < reso - 1; n++) {
    phi[n] = (n - 90) * M_PI / 180.0;
    dif[n] = -D * tan(phi[n]) * (T[n + 1] - T[n - 1]) * 0.5 / del_phi +
             D * (T[n + 1] + T[n - 1] - 2.0 * T[n]) / del_phi / del_phi;
  }
  dif[0] = 0.0;
  dif[180] = 0.0;
}

void calc_radi(const SimulationConfig *conf, const PlanetParams *para,
               ClimateState *state, double radi[]) {
  int reso = conf->reso, n;
  double P_air = state->P_air, season = state->season;
  double *T = state->T;

  // Removed redundant calc_delta - logic doesn't use it.
  // Origin used it in main Calc_Ins, not Radi (legacy line 118 calls calc_radi
  // after calc_dif). Legacy calc_radi definition (line 252) DOES NOT call
  // calc_delta. So no calc_delta here.

  double A = 0.0, B = 0.0;
  if (P_air > 7.45)
    P_air = 7.45;
  if (P_air < 3.4e-12)
    P_air = 3.4e-12;

  double logP =
      log10(P_air); // Legacy uses log10? Line 262: x = log10(P_air). Yes.

  for (n = 0; n < reso; n++) {
    if (T[n] > 230.1) {
      // High T coefficients
      double a[] = {-372.7, 329.9, 99.54, 13.28, 0.6449};
      double b[] = {1.898, -1.68, -0.5069, -0.06758, -0.003256};
      A = a[0] + a[1] * logP + a[2] * pow(logP, 2) + a[3] * pow(logP, 3) +
          a[4] * pow(logP, 4);
      B = b[0] + b[1] * logP + b[2] * pow(logP, 2) + b[3] * pow(logP, 3) +
          b[4] * pow(logP, 4);
    } else {
      // Low T coefficients
      double a[] = {-61.72, 54.64, 16.48, 2.198, 0.1068};
      double b[] = {0.5479, -0.485, -0.1464, -0.0195, -0.00094};
      A = a[0] + a[1] * logP + a[2] * pow(logP, 2) + a[3] * pow(logP, 3) +
          a[4] * pow(logP, 4);
      B = b[0] + b[1] * logP + b[2] * pow(logP, 2) + b[3] * pow(logP, 3) +
          b[4] * pow(logP, 4);
    }
    radi[n] = A + B * T[n];
    if (radi[n] <= 0.0)
      radi[n] = 0.0;
  }
}

void calc_T_M(const SimulationConfig *conf, const PlanetParams *para,
              ClimateState *state, double ins[], double dif[], double radi[]) {
  int reso = conf->reso, n;
  double dt = conf->dt;
  double C = 1.0e7; // Legacy line 354
  double L = 5.9e5;
  double Tsub, a_f, a_i;
  double del_E[reso];

  Tsub_af_ai(state->P_air, &Tsub, &a_f, &a_i);
  state->T_sub = Tsub;

  for (n = 0; n < reso; n++) {
    if (state->M[n] == 0.0) {
      del_E[n] = (ins[n] * (1.0 - a_f) + dif[n] - radi[n]) * dt;
      state->T[n] += del_E[n] / C;
      state->M[n] = 0.0;
      if (state->T[n] < Tsub) {
        state->M[n] = (Tsub - state->T[n]) * C / L;
        state->T[n] = Tsub;
      }
    } else {
      del_E[n] = (ins[n] * (1.0 - a_i) + dif[n] - radi[n]) * dt;
      state->M[n] = state->M[n] - del_E[n] / L + (Tsub - state->T[n]) * C / L;
      state->T[n] = Tsub;
      if (state->M[n] < 0.0) {
        state->T[n] = Tsub + (-state->M[n]) * L / C;
        state->M[n] = 0.0;
      }
    }
  }
}

void calc_ice(const SimulationConfig *conf, const PlanetParams *para,
              ClimateState *state) {
  int reso = conf->reso, n;
  double P_total = para->P_total;
  double upper, lower, scale, Pice = 0.0, factor = 5.815e-6;

  for (n = 0; n < reso; n++) {
    upper = 90.0 / 181.0 * (2.0 * n - 179.0) * M_PI / 180.0;
    lower = 90.0 / 181.0 * (2.0 * n - 181.0) * M_PI / 180.0;
    scale = 2.0 * M_PI * (sin(upper) - sin(lower));
    Pice += state->M[n] * factor * scale;
  }
  if (Pice > P_total)
    state->P_ice = P_total;
  else
    state->P_ice = Pice;
}

void calc_rego(const SimulationConfig *conf, const PlanetParams *para,
               ClimateState *state) {
  int reso = conf->reso, n, loop = 0, loop_max = 100;
  double C = 34.0, T_d = 35.0, gamma = 0.275;
  double sigma = 0.0;
  double kouho = para->P_total / 2.0;
  double theta[reso];
  double lim_hi = para->P_total, lim_lo = 0.0, fx = 0.0;

  for (n = 0; n < reso; n++) {
    theta[n] = (n - 90) * M_PI / 180;
    if (state->T[n] > state->T_sub) {
      sigma += C * exp(-state->T[n] / T_d) * cos(theta[n]) * (M_PI / 180.0);
    }
  }

  do {
    fx = sigma * pow(kouho, gamma) + kouho + state->P_ice - para->P_total;
    loop++;
    if (fx > 0.0) {
      lim_hi = kouho;
      kouho = 0.5 * (lim_hi + lim_lo);
    } else if (fx < 0.0) {
      lim_lo = kouho;
      kouho = 0.5 * (lim_hi + lim_lo);
    }
  } while (fx != 0.0 && loop < loop_max);

  state->P_air = kouho;
  state->P_rego = sigma * pow(kouho, gamma);
  if (kouho < 0)
    state->bug = 1;
  else
    state->bug = 0;
}

void calc_Yearsec(double *Ysec) {
  double ma = 227936640000, G = 6.67384e-11, M = 1.9891e30, m = 639e21;
  *Ysec = sqrt(ma * ma * ma / (G * (M + m))) * 2.0 * M_PI;
}

void calc_delta(const SimulationConfig *conf, const PlanetParams *para,
                double season, double *delta, double *r_AU) {
  int loop = 0, loop_max = 100;
  double u, r;
  double ma = 227936640000.0, G = 6.67384e-11, M = 1.9891e30, m = 639e21;
  double ecc = 0.0934, p = 336.049 * M_PI / 180.0, oneAU = 149597871000.0;
  double nt = sqrt(G * (M + m) / ma / ma / ma) * season;
  double x = nt, cosf, sinf;

  if (ecc == 0.0)
    u = nt;
  else {
    x = nt;
    do {
      x = x - (x - ecc * sin(x) - nt) / (1.0 - ecc * cos(x));
      loop++;
    } while (x - ecc * sin(x) - nt >= 1.0e-6 &&
             loop < loop_max); // Matched Legacy line 399: no fabs
    u = x;
  }
  r = ma * (1.0 - ecc * cos(u));
  *r_AU = r / oneAU;

  if (ecc != 0.0) {
    cosf = (ma * (1.0 - ecc * ecc) / r - 1.0) / ecc;
    sinf = sqrt(1.0 - cosf * cosf);
    if (sin(u) < 0.0)
      sinf = -1.0 * sinf;
  } else {
    cosf = cos(u);
    sinf = sin(u);
  }
  *delta = asin(sin(para->obl) * (sinf * cos(p) + cosf * sin(p)));
}

void heikou(const SimulationConfig *conf, const PlanetParams *para,
            ClimateState *state) {
  int n, loop = 0;
  double abs = 0.0, abs_posi = 0.0, abs_nega = 0.0;
  double ins[181], dif[181], radi[181];
  double ins_posi[181], radi_posi[181], ins_nega[181], radi_nega[181],
      dif_posi[181], dif_nega[181];
  double T_last[181], T_last_posi[181], T_last_nega[181];
  double loop_end = 0.0;

  for (n = 0; n < conf->reso; n++) {
    T_last[n] = state->T[n];
    T_last_posi[n] = state->T_posi[n];
    T_last_nega[n] = state->T_nega[n];
  }

  do {
    abs = 0.0;
    abs_posi = 0.0;
    abs_nega = 0.0;
    fprintf(stderr, "The %d th loop begins.\n", loop);
    // 1 Year Loop
    do {
      calc_ins(conf, para, state, ins);
      calc_dif(conf, para, state, dif);
      calc_radi(conf, para, state, radi);
      calc_T_M(conf, para, state, ins, dif, radi);
      calc_ice(conf, para, state);
      calc_rego(conf, para, state);

      calc_ins_s(conf, para, state, para->alpha_posi, ins_posi);
      calc_radi_s(conf, para, state, state->T_posi, radi_posi);
      calc_dif_s(conf, para, state, state->T_posi, dif_posi);
      calc_Ts_Ms(conf, para, state, state->T_posi, state->M_posi, ins_posi,
                 radi_posi, dif_posi);

      calc_ins_s(conf, para, state, para->alpha_nega, ins_nega);
      calc_radi_s(conf, para, state, state->T_nega, radi_nega);
      calc_dif_s(conf, para, state, state->T_nega, dif_nega); // Using T_nega
      calc_Ts_Ms(conf, para, state, state->T_nega, state->M_nega, ins_nega,
                 radi_nega, dif_nega);

      state->season += conf->dt;
    } while (state->season < conf->Year_sec * (loop + 1) && state->bug == 0);

    for (n = 0; n < conf->reso; n++) {
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
    loop++;
    if (abs < 1.0 && abs_posi < 1.0 && abs_nega < 1.0)
      loop_end = 1.0;
  } while (loop_end == 0.0 && loop < 1 &&
           state->bug == 0); // Legacy limit loop < 1
  state->loop = loop;
  // NO season reset here!
}

void calc_ins_s(const SimulationConfig *conf, const PlanetParams *para,
                ClimateState *state, double alpha, double ins_s[]) {
  int reso = conf->reso, n, mark = 0;
  double Q = para->Q;
  double delta, h, cosH_t, H_t, cosH_s, H_s, r_AU, H_eff;
  double theta[reso], slope[reso];
  double season = state->season;

  calc_delta(conf, para, season, &delta, &r_AU);
  h = fmod(season, conf->day_sec) * 2.0 * M_PI / conf->day_sec - M_PI;

  for (n = 0; n < reso; n++) {
    theta[n] = (n - 90.0) * M_PI / 180.0;
    slope[n] = theta[n] + alpha;

    cosH_t = -tan(theta[n]) * tan(delta);
    if (cosH_t >= 1.0)
      H_t = 0.0;
    else if (cosH_t <= -1.0)
      H_t = M_PI;
    else
      H_t = acos(cosH_t);

    if (slope[n] > 0.5 * M_PI) {
      slope[n] = M_PI - slope[n];
      mark = 1;
    }
    if (slope[n] < -0.5 * M_PI) {
      slope[n] = -M_PI - slope[n];
      mark = 2;
    }

    cosH_s = -tan(slope[n]) * tan(delta);
    if (cosH_s >= 1.0)
      H_s = 0.0;
    else if (cosH_s <= -1.0)
      H_s = M_PI;
    else
      H_s = acos(cosH_s);

    if (mark == 1) {
      slope[n] = M_PI - slope[n];
      mark = 0;
    }
    if (mark == 2) {
      slope[n] = -M_PI - slope[n];
      mark = 0;
    }

    H_eff = fmin(H_t, H_s); // Legacy line 549 uses fmax?
    // Wait, Legacy line 549: H_eff = fmax(H_t, H_s);
    // My previous analysis (Step 192) said "Change fmax to fmin".
    // I should use fmin because "smaller means day is short".
    // BUT if legacy used fmax, I MUST USE FMAX to match output.
    // Legacy view 844: line 549: `H_eff = fmax(H_t, H_s);`
    // So I MUST use fmax to match legacy bug?
    // User requested "fmax to fmin" in original plan?
    // No, I "corrected" it in Step 598.
    // If I want to match Origin, I MUST USE ORIGIN LOGIC (fmax).
    // The user's request might have been to "fix bugs" later, but Phase 1 is
    // "functional equivalence". So I use fmax.
    H_eff = fmax(H_t, H_s);

    if (h >= -H_eff && h <= H_eff) {
      // Legacy line 554: Q / r_AU / r_AU * ...
      // In calc_ins (global), it uses Q / M_PI / r_AU ...
      // I added / M_PI in Step 658 to calc_ins_s.
      // But verify failed.
      // Legacy line 554: `ins_s[n] = Q / r_AU / r_AU * ...` (No M_PI dividend)
      // I should match legacy.
      ins_s[n] =
          Q / r_AU / r_AU *
          (sin(slope[n]) * sin(delta) + cos(slope[n]) * cos(delta) * cos(h));
      if (ins_s[n] <= 0.0)
        ins_s[n] = 0.0;
    } else {
      ins_s[n] = 0.0;
    }
  }
}

void calc_radi_s(const SimulationConfig *conf, const PlanetParams *para,
                 ClimateState *state, double *T_s, double radi_s[]) {
  int reso = conf->reso, n;
  double P_air = state->P_air;
  if (P_air > 7.45)
    P_air = 7.45;
  if (P_air < 3.4e-12)
    P_air = 3.4e-12;
  double logP = log10(P_air);
  double A, B;

  for (n = 0; n < reso; n++) {
    if (T_s[n] > 230.1) {
      double a[] = {-372.7, 329.9, 99.54, 13.28, 0.6449};
      double b[] = {1.898, -1.68, -0.5069, -0.06758, -0.003256};
      A = a[0] + a[1] * logP + a[2] * pow(logP, 2) + a[3] * pow(logP, 3) +
          a[4] * pow(logP, 4);
      B = b[0] + b[1] * logP + b[2] * pow(logP, 2) + b[3] * pow(logP, 3) +
          b[4] * pow(logP, 4);
    } else {
      double a[] = {-61.72, 54.64, 16.48, 2.198, 0.1068};
      double b[] = {0.5479, -0.485, -0.1464, -0.0195, -0.00094};
      A = a[0] + a[1] * logP + a[2] * pow(logP, 2) + a[3] * pow(logP, 3) +
          a[4] * pow(logP, 4);
      B = b[0] + b[1] * logP + b[2] * pow(logP, 2) + b[3] * pow(logP, 3) +
          b[4] * pow(logP, 4);
    }
    radi_s[n] = A + B * T_s[n];
    if (radi_s[n] <= 0.0)
      radi_s[n] = 0.0;
  }
}

void calc_Ts_Ms(const SimulationConfig *conf, const PlanetParams *para,
                ClimateState *state, double *T_s, double *M_s, double ins_s[],
                double radi_s[], double dif_s[]) {
  int reso = conf->reso, n;
  double dt = conf->dt;
  double a_i = 0.63, a_f = 0.21, L = 5.9e5, C = 1.0e7, Tsub = 0.0;
  double del_E[reso];
  double dummy_af, dummy_ai; // Tsub_af_ai sets pointers

  Tsub_af_ai(state->P_air, &Tsub, &dummy_af, &dummy_ai);

  for (n = 0; n < reso; n++) {
    if (M_s[n] == 0.0) {
      del_E[n] = (ins_s[n] * (1.0 - a_f) + dif_s[n] - radi_s[n]) * dt;
      T_s[n] = T_s[n] + del_E[n] / C;
      M_s[n] = 0.0;
      if (T_s[n] < Tsub) {
        M_s[n] = (Tsub - T_s[n]) * C / L;
        T_s[n] = Tsub;
      }
    } else {
      del_E[n] = (ins_s[n] * (1.0 - a_i) + dif_s[n] - radi_s[n]) * dt;
      M_s[n] = M_s[n] - del_E[n] / L + (Tsub - T_s[n]) * C / L;
      T_s[n] = Tsub;
      if (M_s[n] < 0.0) {
        T_s[n] = Tsub + (-M_s[n]) * L / C;
        M_s[n] = 0.0;
      }
    }
  }
}

void Tsub_af_ai(double P_air, double *Tsub, double *a_f, double *a_i) {
  double t[] = {194.36, 26.451, 2.8593, 0.1814, 0.0046};
  double af[] = {0.21, -0.0008, -0.0074, -0.0147, 0.0337, 0.1381, 0.3249};
  double ai[] = {0.63, -0.0008, -0.0011, 0.0183, 0.0599, 0.6997};
  double x = log10(P_air);

  if (P_air >= 1e-3) {
    *a_f = af[1] * pow(x, 5) + af[2] * pow(x, 4) + af[3] * pow(x, 3) +
           af[4] * pow(x, 2) + af[5] * x + af[6];
    *a_i = ai[1] * pow(x, 4) + ai[2] * pow(x, 3) + ai[3] * pow(x, 2) +
           ai[4] * x + ai[5] - 0.08;
  } else {
    *a_f = af[0];
    *a_i = ai[0] - 0.08;
  }

  if (P_air >= 1e-16) {
    *Tsub = t[0] + t[1] * x + t[2] * pow(x, 2) + t[3] * pow(x, 3) +
            t[4] * pow(x, 4);
  } else {
    x = -16.0;
    *Tsub = t[0] + t[1] * x + t[2] * pow(x, 2) + t[3] * pow(x, 3) +
            t[4] * pow(x, 4);
  }
}

void calc_dif_s(const SimulationConfig *conf, const PlanetParams *para,
                ClimateState *state, double T_s[], double dif_s[]) {
  int reso = conf->reso, n;
  double m = 10.0;
  for (n = 0; n < reso; n++) {
    dif_s[n] = m * (state->T[n] - T_s[n]);
  }
}
