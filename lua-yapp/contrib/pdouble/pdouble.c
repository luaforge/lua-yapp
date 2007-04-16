/***********************************************************************
 pdouble.c - Contains a routine to print double-precision floating
    point numbers and associated routines, written by Fred Bayer,
    author of LispMe.  It is available from
    http://www.lispme.de/lispme/gcc/tech.html
     
 This version was formatted and tweaked slightly by Warren Young
    <tangent@cyberport.com>
    
 This routine is in the Public Domain.
 
 Code edited 2001-01-14 by Ben Combee <bcombee@metrowerks.com> to
 work with CodeWarrior for Palm OS 7 and 8.
***********************************************************************/

#include "pdouble.h"

/**********************************************************************/
/* Formatting parameters                                              */
/**********************************************************************/
#define NUM_DIGITS   15
#define MIN_FLOAT    4
#define ROUND_FACTOR 1.0000000000000005 /* NUM_DIGITS zeros */


/**********************************************************************/
/* FP conversion constants                                            */
/**********************************************************************/
static double pow1[] = {
    1e256, 1e128, 1e064,
    1e032, 1e016, 1e008,
    1e004, 1e002, 1e001
};

static double pow2[] = {
    1e-256, 1e-128, 1e-064,
    1e-032, 1e-016, 1e-008,
    1e-004, 1e-002, 1e-001
};

void
printDouble(double x, char *s)
{
    FlpCompDouble fcd;
    short e, e1, i;
    double *pd, *pd1;
    char sign = '\0';
    short dec = 0;

    /*------------------------------------------------------------------*/
    /* Round to desired precision                                       */
    /* (this doesn't always provide a correct last digit!)              */
    /*------------------------------------------------------------------*/
    x = x * ROUND_FACTOR;

    /*------------------------------------------------------------------*/
    /* check for NAN, +INF, -INF, 0                                     */
    /*------------------------------------------------------------------*/
    fcd.d = x;
    if ((fcd.ul[0] & 0x7ff00000) == 0x7ff00000)
        if (fcd.fdb.manH == 0 && fcd.fdb.manL == 0)
            if (fcd.fdb.sign)
                StrCopy(s, "[-inf]");
            else
                StrCopy(s, "[inf]");
        else
            StrCopy(s, "[nan]");
    else if (FlpIsZero(fcd))
        StrCopy(s, "0");
    else {
        /*----------------------------------------------------------------*/
        /* Make positive and store sign                                   */
        /*----------------------------------------------------------------*/
        if (FlpGetSign(fcd)) {
            *s++ = '-';
            FlpSetPositive(fcd);
        }

        if ((unsigned) fcd.fdb.exp < 0x3ff) {   /* meaning x < 1.0 */
            /*--------------------------------------------------------------*/
            /* Build negative exponent                                      */
            /*--------------------------------------------------------------*/
            for (e = 1, e1 = 256, pd = pow1, pd1 = pow2; e1;
                 e1 >>= 1, ++pd, ++pd1)
                if (*pd1 > fcd.d) {
                    e += e1;
                    fcd.d = fcd.d * *pd;
                }
            fcd.d = fcd.d * 10.0;

            /*--------------------------------------------------------------*/
            /* Only print big exponents                                     */
            /*--------------------------------------------------------------*/
            if (e <= MIN_FLOAT) {
                *s++ = '0';
                *s++ = '.';
                dec = -1;
                while (--e)
                    *s++ = '0';
            }
            else
                sign = '-';
        }
        else {
            /*--------------------------------------------------------------*/
            /* Build positive exponent                                      */
            /*--------------------------------------------------------------*/
            for (e = 0, e1 = 256, pd = pow1, pd1 = pow2; e1;
                 e1 >>= 1, ++pd, ++pd1)
                if (*pd <= fcd.d) {
                    e += e1;
                    fcd.d = fcd.d * *pd1;
                }
            if (e < NUM_DIGITS)
                dec = e;
            else
                sign = '+';
        }

        /*----------------------------------------------------------------*/
        /* Extract decimal digits of mantissa                             */
        /*----------------------------------------------------------------*/
        for (i = 0; i < NUM_DIGITS; ++i, --dec) {
            Int32 d = fcd.d;
            *s++ = d + '0';
            if (!dec)
                *s++ = '.';
            fcd.d = fcd.d - (double)d;
            fcd.d = fcd.d * 10.0;
        }

        /*----------------------------------------------------------------*/
        /* Remove trailing zeros and decimal point                        */
        /*----------------------------------------------------------------*/
        while (s[-1] == '0')
            *--s = '\0';
        if (s[-1] == '.')
            *--s = '\0';

        /*----------------------------------------------------------------*/
        /* Append exponent                                                */
        /*----------------------------------------------------------------*/
        if (sign) {
            *s++ = 'e';
            *s++ = sign;
            StrIToA(s, e);
        }
        else
            *s = '\0';
    }
}
