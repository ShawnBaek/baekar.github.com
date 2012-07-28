#include <math.h>

int arUtilMatMul( double s1[3][4], double s2[3][4], double d[3][3] )
{
    int     i, j;

    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 3; i++) {
            d[j][i] = s1[j][0] * s2[0][i]
                    + s1[j][1] * s2[1][i]
                    + s1[j][2] * s2[2][i];
        }
    }

    return 0;
}

int arGetAngle( double rot[3][3], double *wa, double *wb, double *wc )
{
    double      a, b, c;
    double      sina, cosa, sinb, cosb, sinc, cosc;
#if CHECK_CALC
double   w[3];
int      i;
for(i=0;i<3;i++) w[i] = rot[i][0];
for(i=0;i<3;i++) rot[i][0] = rot[i][1];
for(i=0;i<3;i++) rot[i][1] = rot[i][2];
for(i=0;i<3;i++) rot[i][2] = w[i];
#endif

    if( rot[2][2] > 1.0 ) {
        /* printf("cos(beta) = %f\n", rot[2][2]); */
        rot[2][2] = 1.0;
    }
    else if( rot[2][2] < -1.0 ) {
        /* printf("cos(beta) = %f\n", rot[2][2]); */
        rot[2][2] = -1.0;
    }
    cosb = rot[2][2];
    b = acos( cosb );
    sinb = sin( b );
    if( b >= 0.000001 || b <= -0.000001) {
        cosa = rot[0][2] / sinb;
        sina = rot[1][2] / sinb;
        if( cosa > 1.0 ) {
            /* printf("cos(alph) = %f\n", cosa); */
            cosa = 1.0;
            sina = 0.0;
        }
        if( cosa < -1.0 ) {
            /* printf("cos(alph) = %f\n", cosa); */
            cosa = -1.0;
            sina =  0.0;
        }
        if( sina > 1.0 ) {
            /* printf("sin(alph) = %f\n", sina); */
            sina = 1.0;
            cosa = 0.0;
        }
        if( sina < -1.0 ) {
            /* printf("sin(alph) = %f\n", sina); */
            sina = -1.0;
            cosa =  0.0;
        }
        a = acos( cosa );
        if( sina < 0 ) a = -a;

        sinc =  (rot[2][1]*rot[0][2]-rot[2][0]*rot[1][2])
              / (rot[0][2]*rot[0][2]+rot[1][2]*rot[1][2]);
        cosc =  -(rot[0][2]*rot[2][0]+rot[1][2]*rot[2][1])
               / (rot[0][2]*rot[0][2]+rot[1][2]*rot[1][2]);
        if( cosc > 1.0 ) {
            /* printf("cos(r) = %f\n", cosc); */
            cosc = 1.0;
            sinc = 0.0;
        }
        if( cosc < -1.0 ) {
            /* printf("cos(r) = %f\n", cosc); */
            cosc = -1.0;
            sinc =  0.0;
        }
        if( sinc > 1.0 ) {
            /* printf("sin(r) = %f\n", sinc); */
            sinc = 1.0;
            cosc = 0.0;
        }
        if( sinc < -1.0 ) {
            /* printf("sin(r) = %f\n", sinc); */
            sinc = -1.0;
            cosc =  0.0;
        }
        c = acos( cosc );
        if( sinc < 0 ) c = -c;
    }
    else {
        a = b = 0.0;
        cosa = cosb = 1.0;
        sina = sinb = 0.0;
        cosc = rot[0][0];
        sinc = rot[1][0];
        if( cosc > 1.0 ) {
            /* printf("cos(r) = %f\n", cosc); */
            cosc = 1.0;
            sinc = 0.0;
        }
        if( cosc < -1.0 ) {
            /* printf("cos(r) = %f\n", cosc); */
            cosc = -1.0;
            sinc =  0.0;
        }
        if( sinc > 1.0 ) {
            /* printf("sin(r) = %f\n", sinc); */
            sinc = 1.0;
            cosc = 0.0;
        }
        if( sinc < -1.0 ) {
            /* printf("sin(r) = %f\n", sinc); */
            sinc = -1.0;
            cosc =  0.0;
        }
        c = acos( cosc );
        if( sinc < 0 ) c = -c;
    }

    *wa = a;
    *wb = b;
    *wc = c;

    return 0;
}

int arGetRot( double a, double b, double c, double rot[3][3] )
{
    double   sina, sinb, sinc;
    double   cosa, cosb, cosc;
#if CHECK_CALC
    double   w[3];
    int      i;
#endif

    sina = sin(a); cosa = cos(a);
    sinb = sin(b); cosb = cos(b);
    sinc = sin(c); cosc = cos(c);
    rot[0][0] = cosa*cosa*cosb*cosc+sina*sina*cosc+sina*cosa*cosb*sinc-sina*cosa*sinc;
    rot[0][1] = -cosa*cosa*cosb*sinc-sina*sina*sinc+sina*cosa*cosb*cosc-sina*cosa*cosc;
    rot[0][2] = cosa*sinb;
    rot[1][0] = sina*cosa*cosb*cosc-sina*cosa*cosc+sina*sina*cosb*sinc+cosa*cosa*sinc;
    rot[1][1] = -sina*cosa*cosb*sinc+sina*cosa*sinc+sina*sina*cosb*cosc+cosa*cosa*cosc;
    rot[1][2] = sina*sinb;
    rot[2][0] = -cosa*sinb*cosc-sina*sinb*sinc;
    rot[2][1] = cosa*sinb*sinc-sina*sinb*cosc;
    rot[2][2] = cosb;

#if CHECK_CALC
    for(i=0;i<3;i++) w[i] = rot[i][2];
    for(i=0;i<3;i++) rot[i][2] = rot[i][1];
    for(i=0;i<3;i++) rot[i][1] = rot[i][0];
    for(i=0;i<3;i++) rot[i][0] = w[i];
#endif

    return 0;
}

