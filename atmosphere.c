#include "ebm.h"

void calc_dif(const SimulationConfig *conf, const PlanetParams *para,
              ClimateState *state, double dif[]) {
  int reso = conf->reso;
  int n;
  double coefficient = 5.3e-3;
  double D = coefficient * state->P_air;
  double del_phi = M_PI / 180.0;
  double phi[reso];

  for (n = 1; n < reso - 1; n++) {
    phi[n] = (n - 90) * M_PI / 180.0;
    dif[n] =
        -D * tan(phi[n]) * (state->T[n + 1] - state->T[n - 1]) * 0.5 / del_phi +
        D * (state->T[n + 1] + state->T[n - 1] - 2.0 * state->T[n]) / del_phi /
            del_phi;
  }
  dif[0] = 0.0;
  dif[180] = 0.0;
}

void calc_radi(const SimulationConfig *conf, const PlanetParams *para,
               ClimateState *state, double radi[]) {
  int reso = conf->reso;
  int n;
  double x;
  double a[5], b[5];
  double A = 0.0, B = 0.0;
  double P_air = state->P_air;

  if (P_air > 7.45)
    P_air = 7.45;
  if (P_air < 3.4e-12)
    P_air = 3.4e-12;
  x = log10(P_air);

  for (n = 0; n < reso; n++) {
    if (state->T[n] > 230.1) {
      a[0] = -372.7;
      a[1] = 329.9;
      a[2] = 99.54;
      a[3] = 13.28;
      a[4] = 0.6449;
      b[0] = 1.898;
      b[1] = -1.68;
      b[2] = -0.5069;
      b[3] = -0.06758;
      b[4] = -0.003256;
    } else {
      a[0] = -61.72;
      a[1] = 54.64;
      a[2] = 16.48;
      a[3] = 2.198;
      a[4] = 0.1068;
      b[0] = 0.5479;
      b[1] = -0.485;
      b[2] = -0.1464;
      b[3] = -0.0195;
      b[4] = -0.00094;
    }
    A = a[0] + a[1] * x + a[2] * pow(x, 2.0) + a[3] * pow(x, 3.0) +
        a[4] * pow(x, 4.0);
    B = b[0] + b[1] * x + b[2] * pow(x, 2.0) + b[3] * pow(x, 3.0) +
        b[4] * pow(x, 4.0);
    radi[n] = A + B * state->T[n];
    if (radi[n] <= 0.0)
      radi[n] = 0.0;
  }
}

void calc_radi_s(const SimulationConfig *conf, const PlanetParams *para,
                 ClimateState *state, double *T, double radi_s[]) {
  int reso = conf->reso;
  int n;
  double x;
  double a[5], b[5];
  double A = 0.0, B = 0.0;
  double P_air = state->P_air;

  if (P_air > 7.45)
    P_air = 7.45;
  if (P_air < 3.4e-12)
    P_air = 3.4e-12;
  x = log10(P_air);

  for (n = 0; n < reso; n++) {
    if (T[n] > 230.1) {
      a[0] = -372.7;
      a[1] = 329.9;
      a[2] = 99.54;
      a[3] = 13.28;
      a[4] = 0.6449;
      b[0] = 1.898;
      b[1] = -1.68;
      b[2] = -0.5069;
      b[3] = -0.06758;
      b[4] = -0.003256;
    } else {
      a[0] = -61.72;
      a[1] = 54.64;
      a[2] = 16.48;
      a[3] = 2.198;
      a[4] = 0.1068;
      b[0] = 0.5479;
      b[1] = -0.485;
      b[2] = -0.1464;
      b[3] = -0.0195;
      b[4] = -0.00094;
    }
    A = a[0] + a[1] * x + a[2] * pow(x, 2.0) + a[3] * pow(x, 3.0) +
        a[4] * pow(x, 4.0);
    B = b[0] + b[1] * x + b[2] * pow(x, 2.0) + b[3] * pow(x, 3.0) +
        b[4] * pow(x, 4.0);
    radi_s[n] = A + B * T[n];
    if (radi_s[n] <= 0.0)
      radi_s[n] = 0.0;
  }
}

void calc_dif_s(const SimulationConfig *conf, const PlanetParams *para,
                ClimateState *state, double T_s[], double dif_s[]) {
  int reso = conf->reso;
  int n;
  double m = 10.0;

  for (n = 0; n < reso; n++) {
    dif_s[n] = m * (state->T[n] - T_s[n]);
  }
}
