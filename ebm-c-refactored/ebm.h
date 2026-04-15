#ifndef EBM_H
#define EBM_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Struct Definitions

typedef struct {
  int reso;
  double dt;
  double day_sec;
  double Year_sec;
  int step_limit;
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

// Function Prototypes

// insolation.c
void calc_ins(const SimulationConfig *conf, const PlanetParams *para,
              ClimateState *state, double ins[]);
void calc_Yearsec(double *Ysec);
void calc_delta(const SimulationConfig *conf, const PlanetParams *para,
                double season, double *delta, double *r_AU);
void calc_ins_s(const SimulationConfig *conf, const PlanetParams *para,
                ClimateState *state, double alpha, double ins_s[]);

// atmosphere.c
void calc_dif(const SimulationConfig *conf, const PlanetParams *para,
              ClimateState *state, double dif[]);
void calc_radi(const SimulationConfig *conf, const PlanetParams *para,
               ClimateState *state, double radi[]);
void calc_radi_s(const SimulationConfig *conf, const PlanetParams *para,
                 ClimateState *state, double *T, double radi_s[]);
void calc_dif_s(const SimulationConfig *conf, const PlanetParams *para,
                ClimateState *state, double T_s[], double dif_s[]);

// phase_change.c
void calc_T_M(const SimulationConfig *conf, const PlanetParams *para,
              ClimateState *state, double ins[], double dif[], double radi[]);
void calc_ice(const SimulationConfig *conf, const PlanetParams *para,
              ClimateState *state);
void calc_rego(const SimulationConfig *conf, const PlanetParams *para,
               ClimateState *state);
void calc_Ts_Ms(const SimulationConfig *conf, const PlanetParams *para,
                ClimateState *state, double *T, double *M, double ins[],
                double radi[], double dif[]);
void Tsub_af_ai(double P_air, double *Tsub, double *a_f, double *a_i);

// simulation.c
void heikou(const SimulationConfig *conf, const PlanetParams *para,
            ClimateState *state);
void dump_state(const char *dir, const char *filename, int loop, double season,
                double P_air, double P_ice, double P_rego, double T_sub,
                double T[], double M[], double T_posi[], double M_posi[],
                double T_nega[], double M_nega[]);

#endif // EBM_H
