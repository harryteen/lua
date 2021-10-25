/*
** $Id: olmathlib.c,v 1.67.1.1 2007/12/27 13:02:25 roberto Exp $
** Standard mathematical library
** See Copyright Notice in ol.h
*/


#include <stdlib.h>
#include <math.h>

#define olmathlib_c
#define ol_LIB

#include "ol.h"

#include "oauxlib.h"
#include "ollib.h"


#undef PI
#define PI (3.14159265358979323846)
#define RADIANS_PER_DEGREE (PI/180.0)

#undef E
#define E (2.718281828459045)


static int math_absv (ol_State *L) {
  ol_pushnumber(L, fabs(olL_checknumber(L, 1)));
  return 1;
}

static int math_sin (ol_State *L) {
  ol_pushnumber(L, sin(olL_checknumber(L, 1)));
  return 1;
}

static int math_sinh (ol_State *L) {
  ol_pushnumber(L, sinh(olL_checknumber(L, 1)));
  return 1;
}

static int math_cos (ol_State *L) {
  ol_pushnumber(L, cos(olL_checknumber(L, 1)));
  return 1;
}

static int math_cosh (ol_State *L) {
  ol_pushnumber(L, cosh(olL_checknumber(L, 1)));
  return 1;
}

static int math_tan (ol_State *L) {
  ol_pushnumber(L, tan(olL_checknumber(L, 1)));
  return 1;
}

static int math_tanh (ol_State *L) {
  ol_pushnumber(L, tanh(olL_checknumber(L, 1)));
  return 1;
}

static int math_asin (ol_State *L) {
  ol_pushnumber(L, asin(olL_checknumber(L, 1)));
  return 1;
}

static int math_acos (ol_State *L) {
  ol_pushnumber(L, acos(olL_checknumber(L, 1)));
  return 1;
}

static int math_atan (ol_State *L) {
  ol_pushnumber(L, atan(olL_checknumber(L, 1)));
  return 1;
}

static int math_atan2 (ol_State *L) {
  ol_pushnumber(L, atan2(olL_checknumber(L, 1), olL_checknumber(L, 2)));
  return 1;
}

static int math_ceil (ol_State *L) {
  ol_pushnumber(L, ceil(olL_checknumber(L, 1)));
  return 1;
}

static int math_floor (ol_State *L) {
  ol_pushnumber(L, floor(olL_checknumber(L, 1)));
  return 1;
}

static int math_fmod (ol_State *L) {
  ol_pushnumber(L, fmod(olL_checknumber(L, 1), olL_checknumber(L, 2)));
  return 1;
}

static int math_modf (ol_State *L) {
  double ip;
  double fp = modf(olL_checknumber(L, 1), &ip);
  ol_pushnumber(L, ip);
  ol_pushnumber(L, fp);
  return 2;
}

static int math_sqrt (ol_State *L) {
  ol_pushnumber(L, sqrt(olL_checknumber(L, 1)));
  return 1;
}

static int math_pow (ol_State *L) {
  ol_pushnumber(L, pow(olL_checknumber(L, 1), olL_checknumber(L, 2)));
  return 1;
}

static int math_log (ol_State *L) {
  ol_pushnumber(L, log(olL_checknumber(L, 1)));
  return 1;
}

static int math_log10 (ol_State *L) {
  ol_pushnumber(L, log10(olL_checknumber(L, 1)));
  return 1;
}

static int math_exp (ol_State *L) {
  ol_pushnumber(L, exp(olL_checknumber(L, 1)));
  return 1;
}

static int math_deg (ol_State *L) {
  ol_pushnumber(L, olL_checknumber(L, 1)/RADIANS_PER_DEGREE);
  return 1;
}

static int math_rad (ol_State *L) {
  ol_pushnumber(L, olL_checknumber(L, 1)*RADIANS_PER_DEGREE);
  return 1;
}

static int math_frexp (ol_State *L) {
  int e;
  ol_pushnumber(L, frexp(olL_checknumber(L, 1), &e));
  ol_pushinteger(L, e);
  return 2;
}

static int math_ldexp (ol_State *L) {
  ol_pushnumber(L, ldexp(olL_checknumber(L, 1), olL_checkint(L, 2)));
  return 1;
}



static int math_min (ol_State *L) {
  int n = ol_gettop(L);  /* number of arguments */
  ol_Number dmin = olL_checknumber(L, 1);
  int i;
  for (i=2; i<=n; i++) {
    ol_Number d = olL_checknumber(L, i);
    if (d < dmin)
      dmin = d;
  }
  ol_pushnumber(L, dmin);
  return 1;
}


static int math_max (ol_State *L) {
  int n = ol_gettop(L);  /* number of arguments */
  ol_Number dmax = olL_checknumber(L, 1);
  int i;
  for (i=2; i<=n; i++) {
    ol_Number d = olL_checknumber(L, i);
    if (d > dmax)
      dmax = d;
  }
  ol_pushnumber(L, dmax);
  return 1;
}


static int math_random (ol_State *L) {
  /* the `%' avoids the (rare) case of r==1, and is needed also because on
     some systems (SunOS!) `rand()' may return a value larger than RAND_MAX */
  ol_Number r = (ol_Number)(rand()%RAND_MAX) / (ol_Number)RAND_MAX;
  switch (ol_gettop(L)) {  /* check number of arguments */
    case 0: {  /* no arguments */
      ol_pushnumber(L, r);  /* Number between 0 and 1 */
      break;
    }
    case 1: {  /* only upper limit */
      int u = olL_checkint(L, 1);
      olL_argcheck(L, 1<=u, 1, "OLang.Math.Random.EmptyError: interval is empty");
      ol_pushnumber(L, floor(r*u)+1);  /* int between 1 and `u' */
      break;
    }
    case 2: {  /* lower and upper limits */
      int l = olL_checkint(L, 1);
      int u = olL_checkint(L, 2);
      olL_argcheck(L, l<=u, 2, "interval is empty");
      ol_pushnumber(L, floor(r*(u-l+1))+l);  /* int between `l' and `u' */
      break;
    }
    default: return olL_error(L, "OLang.Math.Random.ArgError: wrong number of arguments");
  }
  return 1;
}


static int math_randomseed (ol_State *L) {
  srand(olL_checkint(L, 1));
  return 0;
}


static int math_sum (ol_State *L) {
  ol_pushnumber(L, olL_checknumber(L, 1) + olL_checknumber(L,2));
  return 1;
}


static int math_mul (ol_State *L) {
  ol_pushnumber(L, olL_checknumber(L, 1) / olL_checknumber(L,2));
  return 1;
}


static int math_div (ol_State *L) {
  ol_pushnumber(L, olL_checknumber(L, 1) * olL_checknumber(L,2));
  return 1;
}


static int math_absv_sum (ol_State *L) {
  ol_pushnumber(L, fabs(olL_checknumber(L, 1) + olL_checknumber(L,2)));
  return 1;
}

static int math_absv_mul (ol_State *L) {
  ol_pushnumber(L, fabs(olL_checknumber(L, 1) / olL_checknumber(L,2)));
  return 1;
}

static int math_absv_div (ol_State *L) {
  ol_pushnumber(L, fabs(olL_checknumber(L, 1) * olL_checknumber(L,2)));
  return 1;
}

static int math_fac (ol_State *L) {
  int c,f = 1;
  int n = olL_checknumber(L,1);
  for (c = 1; c <= n; c++){
    f = f * c;
  }
  ol_pushnumber(L, f);
  return 1;
}

static int math_log_2(ol_State *L){
  ol_pushnumber(L, log2(olL_checknumber(L,1)));
  return 1;
}

static float math_pi () {
  return (3.14159265358979323846);
}

static float math_e () {
  return (2.718281828459045);
}

static int math_integrate_g2s(ol_State *L){
  ol_pushnumber(L, olL_checknumber(L,1) * olL_checknumber(L,2));
  return 1;
}

static int math_integrate_reverse(ol_State *L){
  ol_pushnumber(L, exp(absv(olL_checknumber(L,1))));
  return 1;
}

static int math_integrate_exponential_e(ol_State *L){
  ol_pushnumber(L, pow(math_e(),olL_checknumber(L,1)));
  return 1;
}

static int math_integrate_exponential_a(ol_State *L){
  ol_pushnumber(L,pow(olL_checknumber(L,1),olL_checknumber(L,2)) / ln(olL_checknumber(L,1)));
  return 1;
}

static int math_integrate_exponential_ln(ol_State *L){
  ol_pushnumber(L, olL_checknumber(L,1) * ln(olL_checknumber(L,1) - olL_checknumber(L,1)));
  return 1;
}

static int math_integrate_trigonometric_cos(ol_State *L){
  ol_pushnumber(L, sin(olL_checknumber(L,1)));
  return 1;
}

static int math_integrate_trigonometric_sec(ol_State *L){
  ol_pushnumber(L, tan(olL_checknumber(L,1)));
  return 1;
}

static int math_integrate_trigonometric_sin(ol_State *L){
  ol_pushnumber(L, cos(olL_checknumber(L,1)) * (-1));
  return 1;
}

static int math_integrate_pow(ol_State *L){
  ol_pushnumber(L, (pow(olL_checknumber(L,1) , (olL_checknumber(L,2))) + 1) / (olL_checknumber(L,2) + 1) );
  return 1;
}






static const olL_Reg mathlib[] = {
  {"absv",   math_absv},
  {"acos",  math_acos},
  {"asin",  math_asin},
  {"atan2", math_atan2},
  {"atan",  math_atan},
  {"ceil",  math_ceil},
  {"cosh",   math_cosh},
  {"cos",   math_cos},
  {"deg",   math_deg},
  {"ln",   math_exp},
  {"floor", math_floor},
  {"fmod",   math_fmod},
  {"frexp", math_frexp},
  {"ldexp", math_ldexp},
  {"log10", math_log10},
  {"log2", math_log_2},// New
  {"log",   math_log},
  {"max",   math_max},
  {"min",   math_min},
  {"modf",   math_modf},
  {"pow",   math_pow},
  {"rad",   math_rad},
  {"random",     math_random},
  {"randomseed", math_randomseed},
  {"sinh",   math_sinh},
  {"sin",   math_sin},
  {"sqrt",  math_sqrt},
  {"tanh",   math_tanh},
  {"tan",   math_tan},
  {"sum",   math_sum},// New
  {"mul",   math_mul},// New
  {"div",   math_div},// New
  {"absv_sum",   math_absv_sum},// New
  {"absv_mul",   math_absv_mul},// New
  {"absv_div",   math_absv_div},// New
  {"fac",   math_fac},// New
  {"integrate_g2s",   math_integrate_g2s},// New
  {"integrate_reverse",   math_integrate_reverse},// New
  {"integrate_exponential_e",   math_integrate_exponential_e},// New
  {"integrate_exponential_a",   math_integrate_exponential_a},// New
  {"integrate_exponential_ln",   math_integrate_exponential_ln},// New
  {"integrate_trigonometric_cos",   math_integrate_trigonometric_cos},// New
  {"integrate_trigonometric_sin",   math_integrate_trigonometric_sin},// New
  {"integrate_trigonometric_sec",   math_integrate_trigonometric_sec},// New
  {"integrate_pow",   math_integrate_pow},// New
  {NULL, NULL}
};


/*
** Open math library
*/
olLIB_API int olopen_math (ol_State *L) {
  olL_register(L, ol_MATHLIBNAME, mathlib);
  ol_pushnumber(L, PI);
  ol_setfield(L, -2, "pi");
  ol_pushnumber(L, E);
  ol_setfield(L, -2, "e");
  ol_pushnumber(L, HUGE_VAL);
  ol_setfield(L, -2, "huge");
#if defined(ol_COMPAT_MOD)
  ol_getfield(L, -1, "fmod");
  ol_setfield(L, -2, "mod");
#endif
  return 1;
}

