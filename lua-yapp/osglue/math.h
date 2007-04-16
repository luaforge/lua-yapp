/*
 * the PalmOS SDK math.h header does not protect itself against multiple inclusion
 */
#ifndef MATH_H_INCLUDED
#define MATH_H_INCLUDED

#include <PalmOS.h>

double acos(double x) CSEC_MATHRTL;
double asin(double x) CSEC_MATHRTL;
double atan(double x) CSEC_MATHRTL;
double atan2(double y, double x) CSEC_MATHRTL;
double cos(double x) CSEC_MATHRTL;
double sin(double x) CSEC_MATHRTL;
double tan(double x) CSEC_MATHRTL;
void   sincos(double x, double *sinx, double *cosx) CSEC_MATHRTL;
double cosh(double x) CSEC_MATHRTL;
double sinh(double x) CSEC_MATHRTL;
double tanh(double x) CSEC_MATHRTL;
double acosh(double x) CSEC_MATHRTL;
double asinh(double x) CSEC_MATHRTL;
double atanh(double x) CSEC_MATHRTL;
double exp(double x) CSEC_MATHRTL;
double frexp(double x, Int16 *exponent) CSEC_MATHRTL;
double ldexp(double x, Int16 exponent) CSEC_MATHRTL;
double log(double x) CSEC_MATHRTL;
double log10(double x) CSEC_MATHRTL;
double modf(double x, double *intpart) CSEC_MATHRTL;
double expm1(double x) CSEC_MATHRTL;
double log1p(double x) CSEC_MATHRTL;
double logb(double x) CSEC_MATHRTL;
double log2(double x) CSEC_MATHRTL;
double pow(double x, double y) CSEC_MATHRTL;
double sqrt(double x) CSEC_MATHRTL;
double hypot(double x, double y) CSEC_MATHRTL;
double cbrt(double x) CSEC_MATHRTL;
double ceil(double x) CSEC_MATHRTL;
double fabs(double x) CSEC_MATHRTL;
double floor(double x) CSEC_MATHRTL;
double fmod(double x, double y) CSEC_MATHRTL;
Int16    isinf(double x) CSEC_MATHRTL;
Int16    finite(double x) CSEC_MATHRTL;
double scalbn(double x, Int16 exponent) CSEC_MATHRTL;
double drem(double x, double y) CSEC_MATHRTL;
double significand(double x) CSEC_MATHRTL;
double copysign(double x, double y) CSEC_MATHRTL;
Int16    isnan(double x) CSEC_MATHRTL;
Int16    ilogb(double x) CSEC_MATHRTL;
double rint(double x) CSEC_MATHRTL;
double nextafter(double x, double y) CSEC_MATHRTL;
double remainder(double x, double y) CSEC_MATHRTL;
double scalb(double x, double exponent) CSEC_MATHRTL;
double round(double x) CSEC_MATHRTL;
double trunc(double x) CSEC_MATHRTL;
UInt32  signbit(double x) CSEC_MATHRTL;

#include "../contrib/MathLib/MathLib.h"

#define HUGE_VAL 32767 /* XXX machine dependent huge value, math.h -> bits/hugeval*.h */

#endif /* MATH_H_INCLUDED */
