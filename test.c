#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <math.h>
#include <stdio.h>
#include <stdlib.h>


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void calc_ins(double season, double obl, double ins[]);
void calc_dif(double day, double T[], double P_air, double dif[]);
void calc_radi(double season, double T[], double P_air, double radi[]);
void calc_T_M(double day, double P_air, double T[], double M[], double ins[],
              double dif[], double radi[], double dt, double *T_sub);
void calc_ice(double M[], double P_total, double T_sub, double *P_ice);
void calc_rego(double T[], double T_sub, double P_total, double P_ice,
               double *P_rego, double *P_air, double *bug);
void calc_Yearsec(double *Ysec);
void calc_delta(double season, double obl, double *delta, double *r_AU);
void heikou(double obl, double dt, double angle, double P_total,
            double *P_air_fin, double *P_ice_fin, double *P_rego_fin,
            double T[], double M[], double *season_fin, double *bug_fin,
            int *loop_fin, double T_posi[], double M_posi[], double T_nega[],
            double M_nega[]);
void calc_ins_s(double season, double obl, double alpha, double ins_s[]);
void calc_radi_s(double season, double T_s[], double P_air, double radi_s[]);
void calc_Ts_Ms(double P_air, double T_s[], double M_s[], double ins_s[],
                double radi_s[], double dif_s[], double dt);
void Tsub_af_ai(double P_air, double *Tsub, double *a_f, double *a_i);
void calc_dif_s(double day, double T[], double T_s[], double P_air,
                double dif_s[]);

int main(void) {
  int reso = 181, n, loop = 0;
  double T[reso], M[reso], T_sub = 0.0, bug = 0.0;
  double ins[reso], dif[reso], radi[reso];
  double ins_posi[reso], radi_posi[reso], ins_nega[reso], radi_nega[reso],
      dif_posi[reso], dif_nega[reso];
  double T_posi_m[reso], T_nega_m[reso], M_posi_m[reso], M_nega_m[reso];
  double T_m[reso], M_m[reso], Pair_m = 0.0, Pice_m = 0.0, Prego_m = 0.0,
                               day_heikou = 0.0;
  double P_total = 0.130, P_air = P_total, P_ice = 0.0,
         P_rego = 0.0; /*[bar] unit*/
  double T_posi[reso], M_posi[reso], T_nega[reso],
      M_nega[reso]; // Temperature at slope, Mass of dryice at slope
  double season = 0.0, dt = 185.0, day = 0.0, preseason = 0.0, Year_sec,
         day_sec = 60.0 * (60.0 * 24.0 + 40.0);
  double obl = 25.19 * M_PI / 180, alpha_posi = 30.0 * M_PI / 180.0,
         alpha_nega = -1.0 * alpha_posi;
  FILE *fT_posi, *fT_nega, *fM_posi, *fM_nega, *fT_flat, *fM_flat, *fCO2;
  fT_posi = fopen("T_posi.dat", "w");
  fT_nega = fopen("T_nega.dat", "w");
  fM_posi = fopen("M_posi.dat", "w");
  fM_nega = fopen("M_nega.dat", "w");
  fT_flat = fopen("T_flat.dat", "w");
  fM_flat = fopen("M_flat.dat", "w");
  fCO2 = fopen("fCO2.dat", "w");
  for (n = 0; n < reso; n++) {
    T[n] = 250.0;
    M[n] = 0.0;
    T_posi[n] = T[n];
    M_posi[n] = 0.0;
    T_nega[n] = T[n];
    M_nega[n] = 0.0;
    T_posi_m[n] = 0.0;
    T_nega_m[n] = 0.0;
    M_posi_m[n] = 0.0;
    M_nega_m[n] = 0.0;
    T_m[n] = 0.0;
    M_m[n] = 0.0;
  }
  calc_Yearsec(&Year_sec);
  /******************ここまでが初期条件と変数の定義********************************/

  heikou(obl, dt, alpha_posi, P_total, &P_air, &P_ice, &P_rego, T, M, &season,
         &bug, &loop, T_posi, M_posi, T_nega, M_nega);
  day_heikou = (season - fmod(season, day_sec)) / day_sec;

  if (bug == 0.0) {
    preseason = season;
    do {
      day = (season - fmod(season, day_sec)) / day_sec;

      // 全球EBM
      calc_ins(season, obl, ins);
      calc_dif(day, T, P_air, dif);
      calc_radi(season, T, P_air, radi);
      calc_T_M(day, P_air, T, M, ins, dif, radi, dt, &T_sub);
      calc_ice(M, P_total, T_sub, &P_ice);
      calc_rego(T, T_sub, P_total, P_ice, &P_rego, &P_air, &bug);

      // α正，北向き斜面について
      calc_ins_s(season, obl, alpha_posi, ins_posi);
      calc_radi_s(season, T_posi, P_air, radi_posi);
      calc_dif_s(day, T, T_posi, P_air, dif_posi);
      calc_Ts_Ms(P_air, T_posi, M_posi, ins_posi, radi_posi, dif_posi, dt);

      // α負，南向き斜面について
      calc_ins_s(season, obl, alpha_nega, ins_nega);
      calc_radi_s(season, T_nega, P_air, radi_nega);
      calc_dif_s(day, T, T_nega, P_air, dif_nega);
      calc_Ts_Ms(P_air, T_nega, M_nega, ins_nega, radi_nega, dif_posi, dt);
      season = season + dt;

      // 毎日の日射量を記録
      if (preseason > season - day_sec) {
        Pair_m += P_air * dt;
        Prego_m += P_rego * dt;
        Pice_m += P_ice * dt;
        for (n = 0; n < reso; n++) {
          T_posi_m[n] += T_posi[n] * dt;
          T_nega_m[n] += T_nega[n] * dt;
          T_m[n] += T[n] * dt;
          M_posi_m[n] += M_posi[n] * dt;
          M_nega_m[n] += M_nega[n] * dt;
          M_m[n] += M[n] * dt;
        }
      } else {
        fprintf(fCO2, "%g, %g, %g, %g\n", day - day_heikou, Pair_m / day_sec,
                Pice_m / day_sec, Prego_m / day_sec);
        Pair_m = 0.0;
        Prego_m = 0.0;
        Pice_m = 0.0;
        for (n = 0; n < reso; n++) {
          fprintf(fT_posi, "%g, ", T_posi_m[n] / day_sec);
          T_posi_m[n] = 0.0;
          fprintf(fT_nega, "%g, ", T_nega_m[n] / day_sec);
          T_nega_m[n] = 0.0;
          fprintf(fM_posi, "%g, ", M_posi_m[n] / day_sec);
          M_posi_m[n] = 0.0;
          fprintf(fM_nega, "%g, ", M_nega_m[n] / day_sec);
          M_nega_m[n] = 0.0;
          fprintf(fT_flat, "%g, ", T_m[n] / day_sec);
          T_m[n] = 0.0;
          fprintf(fM_flat, "%g, ", M_m[n] / day_sec);
          M_m[n] = 0.0;
        }
        fprintf(fT_posi, "\n");
        fprintf(fT_nega, "\n");
        fprintf(fM_posi, "\n");
        fprintf(fM_nega, "\n");
        fprintf(fT_flat, "\n");
        fprintf(fM_flat, "\n");
        preseason = season;
      }
    } while (season < Year_sec * (loop + 1));
  }
  fclose(fT_posi);
  fclose(fT_nega);
  fclose(fM_posi);
  fclose(fM_nega);
  fclose(fT_flat);
  fclose(fM_flat);
  fclose(fCO2);
  if (bug == 0)
    fprintf(stderr,
            "%g(air), %g(ice), %g(rego), %g(T_sub), %d loops, with no bug\n",
            P_air, P_ice, P_rego, T_sub, loop);
  if (bug == 1)
    fprintf(stderr,
            "%g(air), %g(ice), %g(rego), %g(T_sub), %d loops, with bug!!!\n",
            P_air, P_ice, P_rego, T_sub, loop);
  return (0);
}
void calc_ins(double season, double obl, double ins[]) {
  int reso = 181, n;
  double Q = 1366.0;
  double delta, h, cosH, H, r_AU, day;
  double theta[reso];
  for (n = 0; n < reso; n++) {
    theta[n] = (n - 90.0) * M_PI / 180.0;
  } // theta definition
  day = (season - fmod(season, 60.0 * (60.0 * 24.0 + 40.0))) /
        (60.0 * (60.0 * 24.0 + 40.0));

  h = fmod(season, 60.0 * (60.0 * 24.0 + 40.0)) * 2.0 * M_PI /
          (60.0 * (60.0 * 24.0 + 40.0)) -
      M_PI;
  calc_delta(season, obl, &delta, &r_AU);

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
void calc_dif(double day, double T[], double P_air, double dif[]) {
  int reso = 181, n;
  double coefficient = 5.3e-3;
  double D = coefficient * P_air; /*P[bar]*/
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
void calc_radi(double season, double T[], double P_air, double radi[]) {
  int reso = 181, n;
  double x;
  double a[5], b[5];
  double A = 0.0, B = 0.0;

  if (P_air > 7.45)
    P_air = 7.45; // A と Bの適用限界の都合
  if (P_air < 3.4e-12)
    P_air = 3.4e-12; // A と Bの適用限界（Bが負にならないように）
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
    radi[n] = A + B * T[n];
    if (radi[n] <= 0.0)
      radi[n] = 0.0;
  }
}
void calc_ice(double M[], double P_total, double T_sub, double *P_ice) {
  int reso = 181, n;
  double upper = 0.0, lower = 0.0, scale = 0.0; // unit of scale is [m^2]
  double Pice = 0.0,
         factor = 5.815e-6; // dimention of parameter "factor" is [bar/kg]

  for (n = 0; n < reso; n++) {
    upper = 90.0 / 181.0 * (2.0 * n - 179.0) * M_PI / 180.0;
    lower = 90.0 / 181.0 * (2.0 * n - 181.0) * M_PI / 180.0;
    scale = 2.0 * M_PI * (sin(upper) - sin(lower));
    Pice += M[n] * factor * scale;
  }
  if (Pice > P_total)
    *P_ice = P_total;
  else
    *P_ice = Pice;
}
void calc_rego(double T[], double T_sub, double P_total, double P_ice,
               double *P_rego, double *P_air, double *bug) {
  int reso = 181, n, loop = 0, loop_max = 100;
  double C = 34.0, T_d = 35.0, gamma = 0.275;
  double sigma = 0.0;
  double kouho = P_total / 2.0;
  double theta[reso];
  double lim_hi = P_total, lim_lo = 0.0, fx = 0.0;

  for (n = 0; n < reso; n++) {
    theta[n] = (n - 90) * M_PI / 180;
    if (T[n] > T_sub) {
      sigma = sigma + C * exp(-T[n] / T_d) * cos(theta[n]) * (M_PI / 180.0);
    }
  }

  do {
    fx = sigma * pow(kouho, gamma) + kouho + P_ice - P_total;
    loop = loop + 1;
    if (fx > 0.0) {
      lim_hi = kouho;
      kouho = 0.5 * (lim_hi + lim_lo);
    } else if (fx < 0.0) {
      lim_lo = kouho;
      kouho = 0.5 * (lim_hi + lim_lo);
    }
  } while (fx != 0.0 && loop < loop_max);

  *P_air = kouho;
  *P_rego = sigma * pow(kouho, gamma);

  if (kouho < 0) {
    *bug = 1;
  } else {
    *bug = 0;
  }
}
void calc_T_M(double day, double P_air, double T[], double M[], double ins[],
              double dif[], double radi[], double dt, double *T_sub) {
  int reso = 181, n;
  double Tsub = 0.0, a_i = 0.0, a_f = 0.0, L = 5.9e5, C = 1.0e7; // L [J/kg]
  double del_E[reso];

  Tsub_af_ai(P_air, &Tsub, &a_f, &a_i);
  *T_sub = Tsub;

  for (n = 0; n < reso; n++) {
    if (M[n] == 0.0) { /*氷が無いとき*/
      del_E[n] = (ins[n] * (1.0 - a_f) + dif[n] - radi[n]) * dt;
      T[n] = T[n] + del_E[n] / C;
      M[n] = 0.0;
      if (T[n] < Tsub) {
        M[n] = (Tsub - T[n]) * C / L;
        T[n] = Tsub;
      }
    } else if (M[n] != 0.0) { /*氷があるとき*/
      del_E[n] = (ins[n] * (1.0 - a_i) + dif[n] - radi[n]) * dt;
      M[n] = M[n] - del_E[n] / L + (Tsub - T[n]) * C / L;
      T[n] = Tsub;
      if (M[n] < 0.0) {
        T[n] = Tsub + (-M[n]) * L / C;
        M[n] = 0.0;
      }
    }
  }
}
void calc_Yearsec(double *Ysec) {
  double ma = 227936640000, G = 6.67384e-11, M = 1.9891e30, m = 639e21;
  *Ysec = sqrt(ma * ma * ma / (G * (M + m))) * 2.0 * M_PI;
}
void calc_delta(double season, double obl, double *delta, double *r_AU) {
  int loop = 0, loop_max = 100;
  double u, r;
  double ma = 227936640000.0, G = 6.67384e-11, M = 1.9891e30, m = 639e21;
  double ecc = 0.0934, p = 336.049 * M_PI / 180.0, oneAU = 149597871000.0;
  double x, nt, cosf, sinf; // x is 候補 of u. nt is constant

  nt = sqrt(G * (M + m) / ma / ma / ma) * season;
  if (ecc == 0.0)
    u = nt;
  else {
    x = nt;
    do {
      x = x - (x - ecc * sin(x) - nt) / (1.0 - ecc * cos(x));
      loop = loop + 1;
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
  *delta = asin(sin(obl) * (sinf * cos(p) + cosf * sin(p)));
}
void heikou(double obl, double dt, double angle, double P_total,
            double *P_air_fin, double *P_ice_fin, double *P_rego_fin,
            double T[], double M[], double *season_fin, double *bug_fin,
            int *loop_fin, double T_posi[], double M_posi[], double T_nega[],
            double M_nega[]) {
  int reso = 181, n, loop = 0;
  double T_sub, abs = 0.0, abs_posi = 0.0, abs_nega = 0.0, bug = 0.0,
                loop_end = 0.0; /*lyはlast year去年*/
  double ins[reso], dif[reso], radi[reso];
  double ins_posi[reso], radi_posi[reso], ins_nega[reso], radi_nega[reso],
      dif_posi[reso], dif_nega[reso];
  double T_last[reso], T_last_posi[reso], T_last_nega[reso];
  double alpha_posi = angle, alpha_nega = -angle;
  double P_air = P_total, P_ice = 0.0, P_rego = 0.0; /*[bar] unit*/
  double season = 0.0, day = 0.0, Year_sec,
         day_sec = 60.0 * (60.0 * 24.0 + 40.0);
  calc_Yearsec(&Year_sec);
  for (n = 0; n < reso; n++) {
    T_last[n] = T[n];
    T_last_posi[n] = T_posi[n];
    T_last_nega[n] = T_nega[n];
  }

  do {
    abs = 0.0;
    abs_posi = 0.0;
    abs_nega = 0.0;
    fprintf(stderr, "The %d th loop begins.\n", loop);
    do {
      calc_ins(season, obl, ins);
      calc_dif(day, T, P_air, dif);
      calc_radi(season, T, P_air, radi);
      calc_T_M(day, P_air, T, M, ins, dif, radi, dt, &T_sub);
      calc_ice(M, P_total, T_sub, &P_ice);
      calc_rego(T, T_sub, P_total, P_ice, &P_rego, &P_air, &bug);

      // α正，北向き斜面について
      calc_ins_s(season, obl, alpha_posi, ins_posi);
      calc_radi_s(season, T_posi, P_air, radi_posi);
      calc_dif_s(day, T, T_posi, P_air, dif_posi);
      calc_Ts_Ms(P_air, T_posi, M_posi, ins_posi, radi_posi, dif_posi, dt);

      // α負，南向き斜面について
      calc_ins_s(season, obl, alpha_nega, ins_nega);
      calc_radi_s(season, T_nega, P_air, radi_nega);
      calc_dif_s(day, T, T_posi, P_air, dif_nega);
      calc_Ts_Ms(P_air, T_nega, M_nega, ins_nega, radi_nega, dif_nega, dt);
      season += dt;
    } while (season < Year_sec * (loop + 1) && bug == 0);

    for (n = 0; n < reso; n++) {
      abs += pow(T_last[n] - T[n], 2.0);
      T_last[n] = T[n];
      abs_posi += pow(T_last_posi[n] - T_posi[n], 2.0);
      T_last_posi[n] = T_posi[n];
      abs_nega += pow(T_last_nega[n] - T_nega[n], 2.0);
      T_last_nega[n] = T_nega[n];
    }
    abs = sqrt(abs);
    abs_posi = sqrt(abs_posi);
    abs_nega = sqrt(abs_nega);
    loop += 1;
    if (abs < 1.0 && abs_posi < 1.0 && abs_nega < 1.0)
      loop_end = 1.0;
  } while (loop_end == 0.0 && loop < 100 && bug == 0);
  day = (season - fmod(season, day_sec)) / day_sec;

  // output//
  *season_fin = season;
  *P_air_fin = P_air;
  *P_ice_fin = P_ice;
  *P_rego_fin = P_rego;
  *bug_fin = bug;
  *loop_fin = loop;
}
void calc_ins_s(double season, double obl, double alpha, double ins_s[]) {
  int reso = 181, n, mark = 0;
  double Q = 1366.0,
         day_sec = 60.0 * (60.0 * 24.0 + 40.0); // slope = theta + alpha
  double delta, h, cosH_t, H_t, cosH_s, H_s, r_AU,
      H_eff; // H_eff は実行的な日照時間
  double theta[reso], slope[reso];

  calc_delta(season, obl, &delta, &r_AU);
  h = fmod(season, day_sec) * 2.0 * M_PI / day_sec - M_PI;

  for (n = 0; n < reso; n++) {
    // 相当緯度の計算
    theta[n] = (n - 90.0) * M_PI / 180.0;
    slope[n] = theta[n] + alpha;

    // 現地の日照時間
    cosH_t = -tan(theta[n]) * tan(delta);
    if (cosH_t >= 1.0)
      H_t = 0.0;
    else if (cosH_t <= -1.0)
      H_t = M_PI;
    else
      H_t = acos(cosH_t);

    // 相当緯度の日照時間
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

    // 実効的な日照時間
    H_eff = fmax(H_t, H_s); // 大きい方が日の出が遅いー＞日が短い

    // 日射量の計算
    if (h >= -H_eff && h <= H_eff) {
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
void calc_radi_s(double season, double T_s[], double P_air, double radi_s[]) {
  int reso = 181, n;
  double x;
  double a[5], b[5];
  double A = 0.0, B = 0.0;

  if (P_air > 7.45)
    P_air = 7.45; // A と Bの適用限界の都合
  if (P_air < 3.4e-12)
    P_air = 3.4e-12; // A と Bの適用限界（Bが負にならないように）
  x = log10(P_air);

  for (n = 0; n < reso; n++) {
    if (T_s[n] > 230.1) {
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
    radi_s[n] = A + B * T_s[n];
    if (radi_s[n] <= 0.0)
      radi_s[n] = 0.0;
  }
}
void calc_Ts_Ms(double P_air, double T_s[], double M_s[], double ins_s[],
                double radi_s[], double dif_s[], double dt) {
  int reso = 181, n;
  double
      a_i = 0.63,
      a_f = 0.21, L = 5.9e5, C = 1.0e7,
      Tsub =
          0.0; // L [J/kg] *heat capacity [J/K] 引用 (Nakamura & Tajika 2003)*/
  double del_E[reso];

  Tsub_af_ai(P_air, &Tsub, &a_f, &a_i);

  for (n = 0; n < reso; n++) {
    if (M_s[n] == 0.0) { // 氷のないとき
      del_E[n] = (ins_s[n] * (1.0 - a_f) + dif_s[n] - radi_s[n]) * dt;
      T_s[n] = T_s[n] + del_E[n] / C;
      M_s[n] = 0.0;
      if (T_s[n] < Tsub) {
        M_s[n] = (Tsub - T_s[n]) * C / L;
        T_s[n] = Tsub;
      }
    } else if (M_s[n] != 0.0) { // 氷が0ではないとき，つまりあるとき
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
void calc_dif_s(double day, double T[], double T_s[], double P_air,
                double dif_s[]) {
  int reso = 181, n;
  double m = 10.0;

  for (n = 0; n < reso; n++) {
    dif_s[n] = m * (T[n] - T_s[n]);
  }
}
