#include "ebm.h"

void calc_ins(const SimulationConfig *conf, const PlanetParams *para,
              ClimateState *state, double ins[]) {
  int reso = conf->reso;
  int n;
  double Q = para->Q;
  double delta, h, cosH, H, r_AU, day;
  double theta[reso];

  for (n = 0; n < reso; n++) {
    theta[n] = (n - 90.0) * M_PI / 180.0;
  }

  // Calculate day based on season
  day = (state->season - fmod(state->season, conf->day_sec)) / conf->day_sec;

  // Calculate hour angle h
  h = fmod(state->season, conf->day_sec) * 2.0 * M_PI / conf->day_sec - M_PI;

  calc_delta(conf, para, state->season, &delta, &r_AU);

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
      fprintf(stderr, "r_AU is negative!");
    }
  }
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
  double x, nt, cosf, sinf;

  nt = sqrt(G * (M + m) / ma / ma / ma) * season;
  if (ecc == 0.0)
    u = nt;
  else {
    x = nt;
    do {
      x = x - (x - ecc * sin(x) - nt) / (1.0 - ecc * cos(x));
      loop = loop + 1;
      // REVERTED to match legacy behavior (removed fabs)
    } while (x - ecc * sin(x) - nt >= 1.0e-6 && loop < loop_max);
    u = x;
  }

  r = ma * (1.0 - ecc * cos(u));
  *r_AU = r / oneAU;

  if (ecc != 0.0) {
    cosf = (ma * (1.0 - ecc * ecc) / r - 1.0) / ecc;
    sinf = sqrt(1.0 - cosf * cosf);
    if (sin(u) < 0.0)
      sinf = -1.0 * sinf;
  } else if (ecc == 0.0) {
    cosf = cos(u);
    sinf = sin(u);
  }
  *delta = asin(sin(para->obl) * (sinf * cos(p) + cosf * sin(p)));
}

void calc_ins_s(const SimulationConfig *conf, const PlanetParams *para,
                ClimateState *state, double alpha, double ins_s[]) {
  int reso = conf->reso;
  int n, mark = 0;
  double Q = para->Q;
  double delta, h, cosH_t, H_t, cosH_s, H_s, r_AU, H_eff;
  double theta[reso], slope[reso];

  calc_delta(conf, para, state->season, &delta, &r_AU);
  h = fmod(state->season, conf->day_sec) * 2.0 * M_PI / conf->day_sec - M_PI;

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

    H_eff = fmax(H_t, H_s); // REVERTED to match legacy behavior (fmax)

    if (h >= -H_eff && h <= H_eff) {
      // REVERTED to match legacy behavior (Q / r_AU / r_AU without M_PI)
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
