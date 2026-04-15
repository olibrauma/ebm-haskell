#include "ebm.h"

void Tsub_af_ai(double P_air, double *Tsub, double *a_f, double *a_i) {
  double t[5], af[7], ai[6];
  double x;
  t[0] = 194.36;
  t[1] = 26.451;
  t[2] = 2.8593;
  t[3] = 0.1814;
  t[4] = 0.0046;
  af[0] = 0.21;
  af[1] = -0.0008;
  af[2] = -0.0074;
  af[3] = -0.0147;
  af[4] = 0.0337;
  af[5] = 0.1381;
  af[6] = 0.3249;
  ai[0] = 0.63;
  ai[1] = -0.0008;
  ai[2] = -0.0011;
  ai[3] = 0.0183;
  ai[4] = 0.0599;
  ai[5] = 0.6997;
  x = log10(P_air);

  if (P_air >= 1e-3) {
    *a_f = af[1] * pow(x, 5.0) + af[2] * pow(x, 4.0) + af[3] * pow(x, 3.0) +
           af[4] * pow(x, 2.0) + af[5] * x + af[6];
    *a_i = ai[1] * pow(x, 4.0) + ai[2] * pow(x, 3.0) + ai[3] * pow(x, 2.0) +
           ai[4] * x + ai[5] - 0.08;
  } else {
    *a_f = af[0];
    *a_i = ai[0] - 0.08;
  }

  if (P_air >= 1e-16) {
    *Tsub = t[0] + t[1] * x + t[2] * pow(x, 2.0) + t[3] * pow(x, 3.0) +
            t[4] * pow(x, 4.0);
  } else {
    x = -16.0;
    *Tsub = t[0] + t[1] * x + t[2] * pow(x, 2.0) + t[3] * pow(x, 3.0) +
            t[4] * pow(x, 4.0);
  }
}

void calc_T_M(const SimulationConfig *conf, const PlanetParams *para,
              ClimateState *state, double ins[], double dif[], double radi[]) {
  int reso = conf->reso;
  int n;
  double Tsub = 0.0, a_i = 0.0, a_f = 0.0, L = 5.9e5, C = 1.0e7;
  double del_E[reso];

  Tsub_af_ai(state->P_air, &Tsub, &a_f, &a_i);
  state->T_sub = Tsub;

  for (n = 0; n < reso; n++) {
    if (state->M[n] == 0.0) {
      del_E[n] = (ins[n] * (1.0 - a_f) + dif[n] - radi[n]) * conf->dt;
      state->T[n] = state->T[n] + del_E[n] / C;
      state->M[n] = 0.0;
      if (state->T[n] < Tsub) {
        state->M[n] = (Tsub - state->T[n]) * C / L;
        state->T[n] = Tsub;
      }
    } else if (state->M[n] != 0.0) {
      del_E[n] = (ins[n] * (1.0 - a_i) + dif[n] - radi[n]) * conf->dt;
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
  int reso = conf->reso;
  int n;
  double upper = 0.0, lower = 0.0, scale = 0.0;
  double Pice = 0.0, factor = 5.815e-6;

  for (n = 0; n < reso; n++) {
    upper = 90.0 / 181.0 * (2.0 * n - 179.0) * M_PI / 180.0;
    lower = 90.0 / 181.0 * (2.0 * n - 181.0) * M_PI / 180.0;
    scale = 2.0 * M_PI * (sin(upper) - sin(lower));
    Pice += state->M[n] * factor * scale;
  }
  if (Pice > para->P_total)
    state->P_ice = para->P_total;
  else
    state->P_ice = Pice;
}

void calc_rego(const SimulationConfig *conf, const PlanetParams *para,
               ClimateState *state) {
  int reso = conf->reso;
  int n, loop = 0, loop_max = 100;
  double C = 34.0, T_d = 35.0, gamma = 0.275;
  double sigma = 0.0;
  double kouho = para->P_total / 2.0;
  double theta[reso];
  double lim_hi = para->P_total, lim_lo = 0.0, fx = 0.0;

  for (n = 0; n < reso; n++) {
    theta[n] = (n - 90) * M_PI / 180;
    if (state->T[n] > state->T_sub) {
      sigma =
          sigma + C * exp(-state->T[n] / T_d) * cos(theta[n]) * (M_PI / 180.0);
    }
  }

  do {
    fx = sigma * pow(kouho, gamma) + kouho + state->P_ice - para->P_total;
    loop = loop + 1;
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

  if (kouho < 0) {
    state->bug = 1;
  } else {
    state->bug = 0;
  }
}

void calc_Ts_Ms(const SimulationConfig *conf, const PlanetParams *para,
                ClimateState *state, double *T, double *M, double ins[],
                double radi[], double dif[]) {
  int reso = conf->reso;
  int n;
  double a_i = 0.63, a_f = 0.21, L = 5.9e5, C = 1.0e7, Tsub = 0.0;
  double del_E[reso];

  Tsub_af_ai(state->P_air, &Tsub, &a_f, &a_i);

  for (n = 0; n < reso; n++) {
    if (M[n] == 0.0) {
      del_E[n] = (ins[n] * (1.0 - a_f) + dif[n] - radi[n]) * conf->dt;
      T[n] = T[n] + del_E[n] / C;
      M[n] = 0.0;
      if (T[n] < Tsub) {
        M[n] = (Tsub - T[n]) * C / L;
        T[n] = Tsub;
      }
    } else if (M[n] != 0.0) {
      del_E[n] = (ins[n] * (1.0 - a_i) + dif[n] - radi[n]) * conf->dt;
      M[n] = M[n] - del_E[n] / L + (Tsub - T[n]) * C / L;
      T[n] = Tsub;
      if (M[n] < 0.0) {
        T[n] = Tsub + (-M[n]) * L / C;
        M[n] = 0.0;
      }
    }
  }
}
