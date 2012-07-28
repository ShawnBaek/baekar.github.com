#include "KatoPoseEstimator.h"

#include <map>
#include <deque>

int arGetAngle( double rot[3][3], double *wa, double *wb, double *wc );
int arGetRot( double a, double b, double c, double rot[3][3] );
int arUtilMatMul( double s1[3][4], double s2[3][4], double d[3][3] );


CKatoPoseEstimator::CKatoPoseEstimator(void)
: MAX_LOOP_COUNT(5), MAX_FIT_ERROR(1.0), QUAD_MARKER_STONE(4)
{

}

CKatoPoseEstimator::~CKatoPoseEstimator(void)
{
}


int CKatoPoseEstimator::arGetInitPose( Results *aResult, double rot[3][3] )
{
    double  wdir[3][3];
    double  w, w1, w2, w3;
    int     dir;
    int     j;

	//dir = aResult->dir;
	dir=0;
    for( j = 0; j < 2; j++ ) {
		w1 = aResult->line[(4-dir+j)%4][0] * aResult->line[(6-dir+j)%4][1]
           - aResult->line[(6-dir+j)%4][0] * aResult->line[(4-dir+j)%4][1];
        w2 = aResult->line[(4-dir+j)%4][1] * aResult->line[(6-dir+j)%4][2]
           - aResult->line[(6-dir+j)%4][1] * aResult->line[(4-dir+j)%4][2];
        w3 = aResult->line[(4-dir+j)%4][2] * aResult->line[(6-dir+j)%4][0]
           - aResult->line[(6-dir+j)%4][2] * aResult->line[(4-dir+j)%4][0];

		   

        wdir[j][0] =  w1*(camera->projectionMatrix[0][1]*camera->projectionMatrix[1][2]-camera->projectionMatrix[0][2]*camera->projectionMatrix[1][1])
                   +  w2*camera->projectionMatrix[1][1]
                   -  w3*camera->projectionMatrix[0][1];
        wdir[j][1] = -w1*camera->projectionMatrix[0][0]*camera->projectionMatrix[1][2]
                   +  w3*camera->projectionMatrix[0][0];
        wdir[j][2] =  w1*camera->projectionMatrix[0][0]*camera->projectionMatrix[1][1];
        w = sqrt( wdir[j][0]*wdir[j][0]
                + wdir[j][1]*wdir[j][1]
                + wdir[j][2]*wdir[j][2] );
        wdir[j][0] /= w;
        wdir[j][1] /= w;
        wdir[j][2] /= w;
    }

	if( check_dir(wdir[0],	&(aResult->vertex[(4-dir)%4])/*marker_info->vertex[(4-dir)%4]*/,
							&(aResult->vertex[(5-dir)%4])/*marker_info->vertex[(5-dir)%4]*/) < 0 ) return -1;
    if( check_dir(wdir[1],	&(aResult->vertex[(7-dir)%4])/*marker_info->vertex[(7-dir)%4]*/,
							&(aResult->vertex[(4-dir)%4])/*marker_info->vertex[(4-dir)%4]*/) < 0 ) return -1;
    if( check_rotation(wdir) < 0 ) return -1;

    wdir[2][0] = wdir[0][1]*wdir[1][2] - wdir[0][2]*wdir[1][1];
    wdir[2][1] = wdir[0][2]*wdir[1][0] - wdir[0][0]*wdir[1][2];
    wdir[2][2] = wdir[0][0]*wdir[1][1] - wdir[0][1]*wdir[1][0];
    w = sqrt( wdir[2][0]*wdir[2][0]
            + wdir[2][1]*wdir[2][1]
            + wdir[2][2]*wdir[2][2] );
    wdir[2][0] /= w;
    wdir[2][1] /= w;
    wdir[2][2] /= w;
/*
    if( wdir[2][2] < 0 ) {
        wdir[2][0] /= -w;
        wdir[2][1] /= -w;
        wdir[2][2] /= -w;
    }
    else {
        wdir[2][0] /= w;
        wdir[2][1] /= w;
        wdir[2][2] /= w;
    }
*/

    rot[0][0] = wdir[0][0];
    rot[1][0] = wdir[0][1];
    rot[2][0] = wdir[0][2];
    rot[0][1] = wdir[1][0];
    rot[1][1] = wdir[1][1];
    rot[2][1] = wdir[1][2];
    rot[0][2] = wdir[2][0];
    rot[1][2] = wdir[2][1];
    rot[2][2] = wdir[2][2];

    return 0;
}

int CKatoPoseEstimator::check_dir( double dir[3], CvPoint2D	*st, CvPoint2D	*ed)
{
	CvMat	*mat_a;			//ARMat     *mat_a;
    double    world[2][3];
    double    camera[2][2];
    double    v[2][2];
    double    h;
    int       i, j;

	// mat_a 행렬 초기화 3x3 실수형
	mat_a = cvCreateMat(3, 3, CV_64FC1);		//mat_a = arMatrixAlloc( 3, 3 );			
		
	
	// 카메라 파라메터 대입
    for(j=0;j<3;j++) 
		for(i=0;i<3;i++) 
			mat_a->data.db[j*3+i] = this->camera->projectionMatrix[j][i];
				//mat_a->m[j*3+i] = cpara[j][i];

	// Inverse
	cvInvert(mat_a, mat_a);		//arMatrixSelfInv( mat_a );
		
    world[0][0] = mat_a->data.db[0]*st->x*10.0
                + mat_a->data.db[1]*st->y*10.0
                + mat_a->data.db[2]*10.0;
    world[0][1] = mat_a->data.db[3]*st->x*10.0
                + mat_a->data.db[4]*st->y*10.0
                + mat_a->data.db[5]*10.0;
    world[0][2] = mat_a->data.db[6]*st->x*10.0
                + mat_a->data.db[7]*st->y*10.0
                + mat_a->data.db[8]*10.0;

	// matrix 해제
	cvReleaseMat(&mat_a);	//arMatrixFree( mat_a );
    
    world[1][0] = world[0][0] + dir[0];
    world[1][1] = world[0][1] + dir[1];
    world[1][2] = world[0][2] + dir[2];

    for( i = 0; i < 2; i++ ) {
        h = this->camera->projectionMatrix[2][0] * world[i][0]
          + this->camera->projectionMatrix[2][1] * world[i][1]
          + this->camera->projectionMatrix[2][2] * world[i][2];
        if( h == 0.0 ) return -1;
        camera[i][0] = (this->camera->projectionMatrix[0][0] * world[i][0]
                      + this->camera->projectionMatrix[0][1] * world[i][1]
                      + this->camera->projectionMatrix[0][2] * world[i][2]) / h;
        camera[i][1] = (this->camera->projectionMatrix[1][0] * world[i][0]
                      + this->camera->projectionMatrix[1][1] * world[i][1]
                      + this->camera->projectionMatrix[1][2] * world[i][2]) / h;
    }

    v[0][0] = ed->x - st->x;
    v[0][1] = ed->y - st->y;
    v[1][0] = camera[1][0] - camera[0][0];
    v[1][1] = camera[1][1] - camera[0][1];

    if( v[0][0]*v[1][0] + v[0][1]*v[1][1] < 0 ) {
        dir[0] = -dir[0];
        dir[1] = -dir[1];
        dir[2] = -dir[2];
    }

    return 0;
}

int CKatoPoseEstimator::check_rotation( double rot[2][3] )
{
    double  v1[3], v2[3], v3[3];
    double  ca, cb, k1, k2, k3, k4;
    double  a, b, c, d;
    double  p1, q1, r1;
    double  p2, q2, r2;
    double  p3, q3, r3;
    double  p4, q4, r4;
    double  w;
    double  e1, e2, e3, e4;
    int     f;

    v1[0] = rot[0][0];
    v1[1] = rot[0][1];
    v1[2] = rot[0][2];
    v2[0] = rot[1][0];
    v2[1] = rot[1][1];
    v2[2] = rot[1][2];
    v3[0] = v1[1]*v2[2] - v1[2]*v2[1];
    v3[1] = v1[2]*v2[0] - v1[0]*v2[2];
    v3[2] = v1[0]*v2[1] - v1[1]*v2[0];
    w = sqrt( v3[0]*v3[0]+v3[1]*v3[1]+v3[2]*v3[2] );
    if( w == 0.0 ) return -1;
    v3[0] /= w;
    v3[1] /= w;
    v3[2] /= w;

    cb = v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
    if( cb < 0 ) cb *= -1.0;
    ca = (sqrt(cb+1.0) + sqrt(1.0-cb)) * 0.5;

    if( v3[1]*v1[0] - v1[1]*v3[0] != 0.0 ) {
        f = 0;
    }
    else {
        if( v3[2]*v1[0] - v1[2]*v3[0] != 0.0 ) {
            w = v1[1]; v1[1] = v1[2]; v1[2] = w;
            w = v3[1]; v3[1] = v3[2]; v3[2] = w;
            f = 1;
        }
        else {
            w = v1[0]; v1[0] = v1[2]; v1[2] = w;
            w = v3[0]; v3[0] = v3[2]; v3[2] = w;
            f = 2;
        }
    }
    if( v3[1]*v1[0] - v1[1]*v3[0] == 0.0 ) return -1;
    k1 = (v1[1]*v3[2] - v3[1]*v1[2]) / (v3[1]*v1[0] - v1[1]*v3[0]);
    k2 = (v3[1] * ca) / (v3[1]*v1[0] - v1[1]*v3[0]);
    k3 = (v1[0]*v3[2] - v3[0]*v1[2]) / (v3[0]*v1[1] - v1[0]*v3[1]);
    k4 = (v3[0] * ca) / (v3[0]*v1[1] - v1[0]*v3[1]);

    a = k1*k1 + k3*k3 + 1;
    b = k1*k2 + k3*k4;
    c = k2*k2 + k4*k4 - 1;

    d = b*b - a*c;
    if( d < 0 ) return -1;
    r1 = (-b + sqrt(d))/a;
    p1 = k1*r1 + k2;
    q1 = k3*r1 + k4;
    r2 = (-b - sqrt(d))/a;
    p2 = k1*r2 + k2;
    q2 = k3*r2 + k4;
    if( f == 1 ) {
        w = q1; q1 = r1; r1 = w;
        w = q2; q2 = r2; r2 = w;
        w = v1[1]; v1[1] = v1[2]; v1[2] = w;
        w = v3[1]; v3[1] = v3[2]; v3[2] = w;
        f = 0;
    }
    if( f == 2 ) {
        w = p1; p1 = r1; r1 = w;
        w = p2; p2 = r2; r2 = w;
        w = v1[0]; v1[0] = v1[2]; v1[2] = w;
        w = v3[0]; v3[0] = v3[2]; v3[2] = w;
        f = 0;
    }

    if( v3[1]*v2[0] - v2[1]*v3[0] != 0.0 ) {
        f = 0;
    }
    else {
        if( v3[2]*v2[0] - v2[2]*v3[0] != 0.0 ) {
            w = v2[1]; v2[1] = v2[2]; v2[2] = w;
            w = v3[1]; v3[1] = v3[2]; v3[2] = w;
            f = 1;
        }
        else {
            w = v2[0]; v2[0] = v2[2]; v2[2] = w;
            w = v3[0]; v3[0] = v3[2]; v3[2] = w;
            f = 2;
        }
    }
    if( v3[1]*v2[0] - v2[1]*v3[0] == 0.0 ) return -1;
    k1 = (v2[1]*v3[2] - v3[1]*v2[2]) / (v3[1]*v2[0] - v2[1]*v3[0]);
    k2 = (v3[1] * ca) / (v3[1]*v2[0] - v2[1]*v3[0]);
    k3 = (v2[0]*v3[2] - v3[0]*v2[2]) / (v3[0]*v2[1] - v2[0]*v3[1]);
    k4 = (v3[0] * ca) / (v3[0]*v2[1] - v2[0]*v3[1]);

    a = k1*k1 + k3*k3 + 1;
    b = k1*k2 + k3*k4;
    c = k2*k2 + k4*k4 - 1;

    d = b*b - a*c;
    if( d < 0 ) return -1;
    r3 = (-b + sqrt(d))/a;
    p3 = k1*r3 + k2;
    q3 = k3*r3 + k4;
    r4 = (-b - sqrt(d))/a;
    p4 = k1*r4 + k2;
    q4 = k3*r4 + k4;
    if( f == 1 ) {
        w = q3; q3 = r3; r3 = w;
        w = q4; q4 = r4; r4 = w;
        w = v2[1]; v2[1] = v2[2]; v2[2] = w;
        w = v3[1]; v3[1] = v3[2]; v3[2] = w;
        f = 0;
    }
    if( f == 2 ) {
        w = p3; p3 = r3; r3 = w;
        w = p4; p4 = r4; r4 = w;
        w = v2[0]; v2[0] = v2[2]; v2[2] = w;
        w = v3[0]; v3[0] = v3[2]; v3[2] = w;
        f = 0;
    }

    e1 = p1*p3+q1*q3+r1*r3; if( e1 < 0 ) e1 = -e1;
    e2 = p1*p4+q1*q4+r1*r4; if( e2 < 0 ) e2 = -e2;
    e3 = p2*p3+q2*q3+r2*r3; if( e3 < 0 ) e3 = -e3;
    e4 = p2*p4+q2*q4+r2*r4; if( e4 < 0 ) e4 = -e4;
    if( e1 < e2 ) {
        if( e1 < e3 ) {
            if( e1 < e4 ) {
                rot[0][0] = p1;
                rot[0][1] = q1;
                rot[0][2] = r1;
                rot[1][0] = p3;
                rot[1][1] = q3;
                rot[1][2] = r3;
            }
            else {
                rot[0][0] = p2;
                rot[0][1] = q2;
                rot[0][2] = r2;
                rot[1][0] = p4;
                rot[1][1] = q4;
                rot[1][2] = r4;
            }
        }
        else {
            if( e3 < e4 ) {
                rot[0][0] = p2;
                rot[0][1] = q2;
                rot[0][2] = r2;
                rot[1][0] = p3;
                rot[1][1] = q3;
                rot[1][2] = r3;
            }
            else {
                rot[0][0] = p2;
                rot[0][1] = q2;
                rot[0][2] = r2;
                rot[1][0] = p4;
                rot[1][1] = q4;
                rot[1][2] = r4;
            }
        }
    }
    else {
        if( e2 < e3 ) {
            if( e2 < e4 ) {
                rot[0][0] = p1;
                rot[0][1] = q1;
                rot[0][2] = r1;
                rot[1][0] = p4;
                rot[1][1] = q4;
                rot[1][2] = r4;
            }
            else {
                rot[0][0] = p2;
                rot[0][1] = q2;
                rot[0][2] = r2;
                rot[1][0] = p4;
                rot[1][1] = q4;
                rot[1][2] = r4;
            }
        }
        else {
            if( e3 < e4 ) {
                rot[0][0] = p2;
                rot[0][1] = q2;
                rot[0][2] = r2;
                rot[1][0] = p3;
                rot[1][1] = q3;
                rot[1][2] = r3;
            }
            else {
                rot[0][0] = p2;
                rot[0][1] = q2;
                rot[0][2] = r2;
                rot[1][0] = p4;
                rot[1][1] = q4;
                rot[1][2] = r4;
            }
        }
    }

    return 0;
}

int CKatoPoseEstimator::arGetNewMatrix( double a, double b, double c,
                    double trans[3], double trans2[3][4], double ret[3][4] )
{
    double   cpara2[3][4];
    double   rot[3][3];
    int      i, j;

    arGetRot( a, b, c, rot );

    if( trans2 != NULL ) {
        for( j = 0; j < 3; j++ ) {
            for( i = 0; i < 4; i++ ) {
				cpara2[j][i] = camera->projectionMatrix[j][0] * trans2[0][i]
                             + camera->projectionMatrix[j][1] * trans2[1][i]
                             + camera->projectionMatrix[j][2] * trans2[2][i];
            }
        }
    }
    else {
        for( j = 0; j < 3; j++ ) {
            for( i = 0; i < 4; i++ ) {
                cpara2[j][i] = camera->projectionMatrix[j][i];
            }
        }
    }

    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 3; i++ ) {
            ret[j][i] = cpara2[j][0] * rot[0][i]
                      + cpara2[j][1] * rot[1][i]
                      + cpara2[j][2] * rot[2][i];
        }
        ret[j][3] = cpara2[j][0] * trans[0]
                  + cpara2[j][1] * trans[1]
                  + cpara2[j][2] * trans[2]
                  + cpara2[j][3];
    }

    return(0);
}

ARTKFloat CKatoPoseEstimator::arModifyMatrix( double rot[3][3], double trans[3],
                             CvPoint3D	vertex[], CvPoint2D pos2d[], int num )
{
    double    factor;
    double    a, b, c;
    double    a1, b1, c1;
    double    a2, b2, c2;
    double    ma = 0.0, mb = 0.0, mc = 0.0;
    double    combo[3][4];
    double    hx, hy, h, x, y;
    double    err, minerr;
    int       t1, t2, t3;
    int       s1 = 0, s2 = 0, s3 = 0;
    int       i, j;

    arGetAngle( rot, &a, &b, &c );

    a2 = a;
    b2 = b;
    c2 = c;
    factor = 10.0*CV_PI/180.0;
    for( j = 0; j < 10; j++ ) {
        minerr = 1000000000.0;
        for(t1=-1;t1<=1;t1++) {
        for(t2=-1;t2<=1;t2++) {
        for(t3=-1;t3<=1;t3++) {
            a1 = a2 + factor*t1;
            b1 = b2 + factor*t2;
            c1 = c2 + factor*t3;
            arGetNewMatrix( a1, b1, c1, trans, NULL, combo );

            err = 0.0;
            for( i = 0; i < num; i++ ) {
                hx = combo[0][0] * vertex[i].x
                   + combo[0][1] * vertex[i].y
                   + combo[0][2] * vertex[i].z
                   + combo[0][3];
                hy = combo[1][0] * vertex[i].x
                   + combo[1][1] * vertex[i].y
                   + combo[1][2] * vertex[i].z
                   + combo[1][3];
                h  = combo[2][0] * vertex[i].x
                   + combo[2][1] * vertex[i].y
                   + combo[2][2] * vertex[i].z
                   + combo[2][3];
                x = hx / h;
                y = hy / h;

                err += (pos2d[i].x - x) * (pos2d[i].x - x)
                     + (pos2d[i].y - y) * (pos2d[i].y - y);
            }

            if( err < minerr ) {
                minerr = err;
                ma = a1;
                mb = b1;
                mc = c1;
                s1 = t1; s2 = t2; s3 = t3;
            }
        }
        }
        }

        if( s1 == 0 && s2 == 0 && s3 == 0 ) factor *= 0.5;
        a2 = ma;
        b2 = mb;
        c2 = mc;
    }

    arGetRot( ma, mb, mc, rot );

/*  printf("factor = %10.5f\n", factor*180.0/MD_PI); */

    return minerr/num;
}


ARTKFloat CKatoPoseEstimator::arGetTransMatSub( double rot[3][3], CvPoint2D *ppos2d[4], CvPoint3D pos3d[],
											/*double ppos2d[][2],double pos3d[][3], */
											int num, double conv[3][4])
{
	CvPoint2D	pos2d[P_MAX];

	CvMat	*mat_a, *mat_b, *mat_c, *mat_d, *mat_e, *mat_f;
    //ARMat   *mat_a, *mat_b, *mat_c, *mat_d, *mat_e, *mat_f;
    double  trans[3];
    double  wx, wy, wz;
    double  ret;
    int     i, j;

	mat_a = cvCreateMat( num*2, 3, CV_64FC1 );
    mat_b = cvCreateMat( 3, num*2 , CV_64FC1 );
    mat_c = cvCreateMat( num*2, 1 , CV_64FC1 );
    mat_d = cvCreateMat( 3, 3, CV_64FC1 );
    mat_e = cvCreateMat( 3, 1, CV_64FC1 );
    mat_f = cvCreateMat( 3, 1, CV_64FC1 );
			//mat_a = arMatrixAlloc( num*2, 3 );
			//mat_b = arMatrixAlloc( 3, num*2 );
			//mat_c = arMatrixAlloc( num*2, 1 );
			//mat_d = arMatrixAlloc( 3, 3 );
			//mat_e = arMatrixAlloc( 3, 1 );
			//mat_f = arMatrixAlloc( 3, 1 );


	// 기준점을 Ideal->observed로 변환하여 pos2d에 저장
	//CvPoint	temp;
	for( i = 0; i < num; i++ ) {
		//camera->ideal2Observ((*ppos2d[i]), &temp);
		//pos2d[i].x = temp.x;
		//pos2d[i].y = temp.y;
		camera->ideal2Observ((*ppos2d[i]), &(pos2d[i]));
		//arParamIdeal2Observ(dist_factor, ppos2d[i][0], ppos2d[i][1],
		//					&pos2d[i][0], &pos2d[i][1]);
  //      }
    }

    for( j = 0; j < num; j++ ) {
        wx = rot[0][0] * pos3d[j].x
           + rot[0][1] * pos3d[j].y
           + rot[0][2] * pos3d[j].z;
        wy = rot[1][0] * pos3d[j].x
           + rot[1][1] * pos3d[j].y
           + rot[1][2] * pos3d[j].z;
        wz = rot[2][0] * pos3d[j].x
           + rot[2][1] * pos3d[j].y
           + rot[2][2] * pos3d[j].z;
		mat_a->data.db[j*6+0] = mat_b->data.db[num*0+j*2] = camera->projectionMatrix[0][0];
        mat_a->data.db[j*6+1] = mat_b->data.db[num*2+j*2] = camera->projectionMatrix[0][1];
        mat_a->data.db[j*6+2] = mat_b->data.db[num*4+j*2] = camera->projectionMatrix[0][2] - pos2d[j].x;
        mat_c->data.db[j*2+0] = wz * pos2d[j].x
               - camera->projectionMatrix[0][0]*wx - camera->projectionMatrix[0][1]*wy - camera->projectionMatrix[0][2]*wz;
        mat_a->data.db[j*6+3] = mat_b->data.db[num*0+j*2+1] = 0.0;
        mat_a->data.db[j*6+4] = mat_b->data.db[num*2+j*2+1] = camera->projectionMatrix[1][1];
        mat_a->data.db[j*6+5] = mat_b->data.db[num*4+j*2+1] = camera->projectionMatrix[1][2] - pos2d[j].y;
        mat_c->data.db[j*2+1] = wz * pos2d[j].y
               - camera->projectionMatrix[1][1]*wy - camera->projectionMatrix[1][2]*wz;
    }
	cvMatMul(mat_b, mat_a, mat_d);
	cvMatMul(mat_b, mat_c, mat_e);
	cvInv(mat_d, mat_d);
	cvMatMul(mat_d, mat_e, mat_f);
    //arMatrixMul( mat_d, mat_b, mat_a );
    //arMatrixMul( mat_e, mat_b, mat_c );
    //arMatrixSelfInv( mat_d );
    //arMatrixMul( mat_f, mat_d, mat_e );
	trans[0] = mat_f->data.db[0];
    trans[1] = mat_f->data.db[1];
    trans[2] = mat_f->data.db[2];

    ret = arModifyMatrix( rot, trans, pos3d, pos2d, num );

    for( j = 0; j < num; j++ ) {
        wx = rot[0][0] * pos3d[j].x
           + rot[0][1] * pos3d[j].y
           + rot[0][2] * pos3d[j].z;
        wy = rot[1][0] * pos3d[j].x
           + rot[1][1] * pos3d[j].y
           + rot[1][2] * pos3d[j].z;
        wz = rot[2][0] * pos3d[j].x
           + rot[2][1] * pos3d[j].y
           + rot[2][2] * pos3d[j].z;
        mat_a->data.db[j*6+0] = mat_b->data.db[num*0+j*2] = camera->projectionMatrix[0][0];
        mat_a->data.db[j*6+1] = mat_b->data.db[num*2+j*2] = camera->projectionMatrix[0][1];
        mat_a->data.db[j*6+2] = mat_b->data.db[num*4+j*2] = camera->projectionMatrix[0][2] - pos2d[j].x;
        mat_c->data.db[j*2+0] = wz * pos2d[j].x
               - camera->projectionMatrix[0][0]*wx - camera->projectionMatrix[0][1]*wy - camera->projectionMatrix[0][2]*wz;
        mat_a->data.db[j*6+3] = mat_b->data.db[num*0+j*2+1] = 0.0;
        mat_a->data.db[j*6+4] = mat_b->data.db[num*2+j*2+1] = camera->projectionMatrix[1][1];
        mat_a->data.db[j*6+5] = mat_b->data.db[num*4+j*2+1] = camera->projectionMatrix[1][2] - pos2d[j].y;
        mat_c->data.db[j*2+1] = wz * pos2d[j].y
               - camera->projectionMatrix[1][1]*wy - camera->projectionMatrix[1][2]*wz;
    }
	cvMatMul(mat_b, mat_a, mat_d);
	cvMatMul(mat_b, mat_c, mat_e);
	cvInv(mat_d, mat_d);
	cvMatMul(mat_d, mat_e, mat_f);
	

    //arMatrixMul( mat_d, mat_b, mat_a );
    //arMatrixMul( mat_e, mat_b, mat_c );
    //arMatrixSelfInv( mat_d );
    //arMatrixMul( mat_f, mat_d, mat_e );
    trans[0] = mat_f->data.db[0];
    trans[1] = mat_f->data.db[1];
    trans[2] = mat_f->data.db[2];

    ret = arModifyMatrix( rot, trans, pos3d, pos2d, num );

	cvReleaseMat(&mat_a);
	cvReleaseMat(&mat_b);
	cvReleaseMat(&mat_c);
	cvReleaseMat(&mat_d);
	cvReleaseMat(&mat_e);
	cvReleaseMat(&mat_f);
    //arMatrixFree( mat_a );
    //arMatrixFree( mat_b );
    //arMatrixFree( mat_c );
    //arMatrixFree( mat_d );
    //arMatrixFree( mat_e );
    //arMatrixFree( mat_f );

    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 3; i++ ) conv[j][i] = rot[j][i];
        conv[j][3] = trans[j];
    }

    return ret;
}

ARTKFloat CKatoPoseEstimator::arGetTransMat3(	double rot[3][3], 
											 CvPoint2D *ppos2d[4], CvPoint3D ppos3d[],
											/*double ppos2d[][2],double ppos3d[][2],*/ 
											int num, double conv[3][4] )
{
	CvPoint3D	pos3d[P_MAX];
	//double	pos3d[P_MAX][3];
	CvPoint3D	pmax, pmin, off;
    //double  off[3], pmax[3], pmin[3];
    double  ret;
    int     i;

	pmax = cvPoint3D(-10000000000.0, -10000000000.0, -10000000000.0);
	pmin = cvPoint3D(10000000000.0,  10000000000.0,  10000000000.0);
    for( i = 0; i < num; i++ ) {
		//cvMax(&pmax, &(ppos3d[i]), &pmax);
		//cvMin(&pmin, &(ppos3d[i]), &pmin);
        if( ppos3d[i].x > pmax.x ) pmax.x = ppos3d[i].x;
        if( ppos3d[i].x < pmin.x ) pmin.x = ppos3d[i].x;
        if( ppos3d[i].y > pmax.y ) pmax.y = ppos3d[i].y;
        if( ppos3d[i].y < pmin.y ) pmin.y = ppos3d[i].y;
		if( ppos3d[i].z > pmax.z ) pmax.z = ppos3d[i].z;
        if( ppos3d[i].z < pmin.z ) pmin.z = ppos3d[i].z;

        //if( ppos3d[i][0] > pmax[0] ) pmax[0] = ppos3d[i][0];
        //if( ppos3d[i][0] < pmin[0] ) pmin[0] = ppos3d[i][0];
        //if( ppos3d[i][1] > pmax[1] ) pmax[1] = ppos3d[i][1];
        //if( ppos3d[i][1] < pmin[1] ) pmin[1] = ppos3d[i][1];
/* ARTK에서 막은것들
        if( ppos3d[i].z > pmax.z ) pmax.z = ppos3d[i].z;
        if( ppos3d[i].z < pmin.z ) pmin.z = ppos3d[i].z;
        if( ppos3d[i][2] > pmax[2] ) pmax[2] = ppos3d[i][2];
        if( ppos3d[i][2] < pmin[2] ) pmin[2] = ppos3d[i][2];
*/
    }

	//cvAddWeighted(&pmax, -0.5, &pmin, -0.5, 0, &off);
    off.x = -(pmax.x + pmin.x) / 2.0;
    off.y = -(pmax.y + pmin.y) / 2.0;
    off.z = -(pmax.z + pmin.z) / 2.0;
    //off[0] = -(pmax[0] + pmin[0]) / 2.0;
    //off[1] = -(pmax[1] + pmin[1]) / 2.0;
    //off[2] = -(pmax[2] + pmin[2]) / 2.0;
    for( i = 0; i < num; i++ ) {
		pos3d[i].x = ppos3d[i].x + off.x;
		pos3d[i].y = ppos3d[i].y + off.y;
        pos3d[i].z = ppos3d[i].z + off.z;

        //pos3d[i][0] = ppos3d[i][0] + off[0];
        //pos3d[i][1] = ppos3d[i][1] + off[1];
        //pos3d[i][2] = 0.0;
/*
        pos3d[i][2] = ppos3d[i][2] + off[2];
*/
    }

    ret = arGetTransMatSub( rot, ppos2d, pos3d, num, conv);

    conv[0][3] = conv[0][0]*off.x + conv[0][1]*off.y + conv[0][2]*off.z + conv[0][3];
    conv[1][3] = conv[1][0]*off.x + conv[1][1]*off.y + conv[1][2]*off.z + conv[1][3];
    conv[2][3] = conv[2][0]*off.x + conv[2][1]*off.y + conv[2][2]*off.z + conv[2][3];

    return ret;
}

//이것만 호출한다.
ARTKFloat CKatoPoseEstimator::calculateTransformationMatrix(CCamera *camera, Results* aResult)
{
	this->camera = camera;

    double  rot[3][3];
	CvPoint2D	*ppos2d[4];
	CvPoint3D	ppos3d[4];
    //double  ppos2d[4][2];
    //double  ppos3d[4][2];
    int     dir;
    double  err;
    int     i;

    if( arGetInitPose( aResult, rot ) < 0 ) return -1;

	dir = aResult->dir;
	ppos2d[0] = &(aResult->vertex[(4-dir)%4]);
	ppos2d[1] = &(aResult->vertex[(5-dir)%4]);
	ppos2d[2] = &(aResult->vertex[(6-dir)%4]);
	ppos2d[3] = &(aResult->vertex[(7-dir)%4]);

	//ppos2d[0][0] = aResult->vertex[(4-dir)%4].x;
 //   ppos2d[0][1] = aResult->vertex[(4-dir)%4].y;
 //   ppos2d[1][0] = aResult->vertex[(5-dir)%4].x;
 //   ppos2d[1][1] = aResult->vertex[(5-dir)%4].y;
 //   ppos2d[2][0] = aResult->vertex[(6-dir)%4].x;
 //   ppos2d[2][1] = aResult->vertex[(6-dir)%4].y;
 //   ppos2d[3][0] = aResult->vertex[(7-dir)%4].x;
 //   ppos2d[3][1] = aResult->vertex[(7-dir)%4].y;
	ppos3d[0].x = 0 - DEFAULT_MARKER_WIDTH/2.0;
    ppos3d[0].y = 0 + DEFAULT_MARKER_WIDTH/2.0;
	ppos3d[0].z = 0;
    ppos3d[1].x = 0 + DEFAULT_MARKER_WIDTH/2.0;
    ppos3d[1].y = 0 + DEFAULT_MARKER_WIDTH/2.0;
    ppos3d[1].z = 0;
	ppos3d[2].x = 0 + DEFAULT_MARKER_WIDTH/2.0;
    ppos3d[2].y = 0 - DEFAULT_MARKER_WIDTH/2.0;
    ppos3d[2].z = 0;
	ppos3d[3].x = 0 - DEFAULT_MARKER_WIDTH/2.0;
    ppos3d[3].y = 0 - DEFAULT_MARKER_WIDTH/2.0;
	ppos3d[3].z = 0;

    for( i = 0; i < MAX_LOOP_COUNT ; i++ ) {
		err = arGetTransMat3( rot, ppos2d, ppos3d, QUAD_MARKER_STONE, aResult->Tcm );
		if( err < MAX_FIT_ERROR ) break;
    }
    return err;
}







ARTKFloat CKatoPoseEstimator::calculateTranformationMatrixMulti(CCamera* camera, 
															Results* results, int marker_num, 
															MultiConfigure* aConfig)
															// aConfig --> num_of_markers 1개,
															//				id, markerWidth, coordOrigin, markerTrasnform** n개
{

	this->camera = camera;

	aConfig->cVisible = false;



	////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////
	/////						marker_id_freq라는 맵에 읽어온 마커(results)의 <id, 개수> 삽입 <-- 마커인식id, 개수이므로 필요없을 듯
	std::map<int, int> marker_id_freq;
	for(int i=0; i<marker_num; i++)
	{
		const int m_patt_id = results[i].ID;
		if(m_patt_id >= 0)
		{
			std::map<int, int>::iterator iter = marker_id_freq.find(m_patt_id);
			if(iter == marker_id_freq.end()) marker_id_freq.insert(std::make_pair<int,int>(m_patt_id,1));
			else ((*iter).second)++;
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////
	/////				config_patt_id라는 데큐에 aConfig 구성 마커들의  <index, id> 삽입
	std::deque<std::pair<int,int> > config_patt_id;
	for(int j=0; j<aConfig->num_of_markers; j++)
		config_patt_id.push_back(std::make_pair<int,int>(j, aConfig->markers[j].id));

	////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////
	/////						m2c_idx  (마커 to 카메라)  라는 맵에 id 삽입
	std::map<int, int> m2c_idx;
	for(int m=0; m<marker_num; m++)						// m: 영상에서 찾은 m번째 마커 
	{
		const int m_patt_id = results[m].ID;			// 영상에서 찾은 m번째 마커의 id
		bool ignore_marker = (m_patt_id < 0);			// 찾은 id가 음수면 true, 양수면 false ==> valid:false / invalid:true
		std::map<int, int>::iterator m_iter = marker_id_freq.find(m_patt_id);		// m_iter: marker_id_freq 맵에서 현재 마커 id 찾음
		if(m_iter != marker_id_freq.end())				// 찾아지면, ignore_marker: frequency가 1 이상인지 확인
			ignore_marker |= ((*m_iter).second > 1);	// 2개 이상의 마커이면 true	// 1개 이하이면 현상유지

		if(!ignore_marker)		// 찾은 id가 양수이거나, 2개 이상이면
		{
			std::deque<std::pair<int,int> >::iterator c_iter = config_patt_id.begin();		// c_iter:	config_patt_id 데큐 탐색자
			if(c_iter != config_patt_id.end()) do													// nested loop: outer- results
																									//				inner- config
			{
				const int patt_id = (*c_iter).second;										// patt_id:	config에서 각 id
				if(results[m].ID == patt_id)												// 찾은 id와 같은 config의 마커를 찾음
				{
					m2c_idx.insert(std::make_pair<int,int>(m,(*c_iter).first));				////// m2c_idx에 <찾은 result의 index, config에서의 index>  삽입
					config_patt_id.erase(c_iter);											// 이건 찾았으니 config_patt_id 자료형에서 삭제
					c_iter = config_patt_id.end();											// 하고, 더이상 탐색 중지

					continue;
				}
				else
				{
					c_iter++;
				}
			}
			while(c_iter != config_patt_id.end());
		}
	}
	//	여기까지 왔으면,	marker_id_freq: <찾은 마커 id, 찾은 개수> 유지
	//						config_patt_id: config에는 있으나 찾아지지 않은 마커 id만 유지
	//						m2c_idx: <찾은 result의 index, config에서의 index> 유지
	/////////////////////////////////////////////////////////////////////////////////////////////////////////

	// ----------------------------------------------------------------------
	const unsigned int n_markers = (unsigned int) m2c_idx.size();			// 이 config에서 찾아진 마커의 수
	const unsigned int n_pts = 4*n_markers;									//	의 4배 <== 포인트 수

	if(n_markers == 0) {
		aConfig->prevF = 0;
		return(-1);	// 여기서 죽으면 이번 config는 하나도 못 찾았다는 뜻
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////					Pose Estimation~!!!   여기가 중요하지....!!
	CvPoint2D	**ppos2d = new CvPoint2D*[n_pts];  //<--n_pts는 그냥 4로 놓으면 될듯
	CvPoint3D	*ppos3d = new CvPoint3D[n_pts];

	ARTKFloat	err=0.0;

	//rpp_vec *ppos2d = NULL, *ppos3d = NULL;
	//ppos2d = (rpp_vec*)malloc(sizeof(rpp_vec) * (n_pts));			// 찾아진 마커 수 * 4(n_ptr) * 3(rpp_vec)
	//ppos3d = (rpp_vec*)malloc(sizeof(rpp_vec) * (n_pts));			// 찾아진 마커 수 * 4(n_ptr) * 3(rpp_vec)
	//memset(ppos2d,0,sizeof(rpp_vec)*n_pts);		// 0으로 초기화
	//memset(ppos3d,0,sizeof(rpp_vec)*n_pts);		// 0으로 초기화

	//const rpp_float iprts_z =  1;

	int p=0;
	int	max_area = -999, max_area_result_idx=0, max_area_config_idx = 0;
	for(std::map<int, int>::iterator iter = m2c_idx.begin();
		iter != m2c_idx.end(); iter++)								// m2c_idex를 탐색 ==> iter(탐색자)
	{
		const int m = (*iter).first;								// m: 현재 찾아진 마커의 인덱스
		const int c = (*iter).second;								// c: 관련된 config의 인덱스

		const int dir = results[m].dir;								// dir: 현재 찾아진 마커의 방향
		const int v_idx[4] = {(4-dir)%4, (5-dir)%4, (6-dir)%4, (7-dir)%4};		// v_idx[4]: 4 방향 인덱스

		for(int i=0; i<4; i++){
			// 2차원 좌표의 homogeneous 좌표
			ppos2d[p+i] = &(results[m].vertex[v_idx[i]]);
			//ppos2d[p+i][0] = (rpp_float) (results[m].vertex[v_idx[i]].x);
			//ppos2d[p+i][1] = (rpp_float) (results[m].vertex[v_idx[i]].y);
			//ppos2d[p+i][2] = (rpp_float) (iprts_z );		

			ppos3d[p+i].x = aConfig->markers[c].pos3d[i][0];		// Configure 좌표계에서 각 점의 위치
			ppos3d[p+i].y = aConfig->markers[c].pos3d[i][1];		// Configure 좌표계에서 각 점의 위치
			ppos3d[p+i].z = aConfig->markers[c].pos3d[i][2];		// Configure 좌표계에서 각 점의 위치


			// Max Area 결과 찾기
			if(results[m].area > max_area) {
				max_area = results[m].area;
				max_area_result_idx = m;
				max_area_config_idx = c;
			}
		}
		p += 4;
	}

	//const ARTKFloat cc[2] = {camera->projectionMatrix[0][2],camera->projectionMatrix[1][2]};
	//const ARTKFloat fc[2] = {camera->projectionMatrix[0][0],camera->projectionMatrix[1][1]};



	// initR의 초기화
	//double	initR[3][3] = {	-0.732607191386725,	-0.0252060382357456,	0.680184571413967,
	//						-0.533312878340816, 0.642181269457273,		-0.550618207206169,
	//						-0.422922887951736,	-0.766138285686534,		-0.483909320981329};
	double	initR[3][3] = {0};

	if(aConfig->prevF) {
		// 이전 프레임에 찾았었으면 그걸 초기 행렬로
		for(int i=0; i<3; ++i) {
			for(int j=0; j<3; ++j) {
				initR[i][j] = aConfig->trans[i][j];
			}
		}
	} else {
		// 못 찾았었으면 제일 큰 마커의 Tcm을 이용

		// Max Area의 Transpose Matrix 계산
		//
		calculateTransformationMatrix(this->camera, &(results[max_area_result_idx]));


		// matrix 곱셈: results[max_area_result_idx].Tcm x itrans ==> rot
		//	max_area_config_idx
		arUtilMatMul(results[max_area_result_idx].Tcm, aConfig->markers[max_area_config_idx].itrans, initR);
	}
	//initR[0][0] = 0.749547308;
	//initR[0][1] = 0.041695269;
	//initR[0][2] = -0.660636312;

	//initR[1][0] = -0.203404048;
	//initR[1][1] = -0.935222017;
	//initR[1][2] = -0.289804369;

	//initR[2][0] = -0.629925096;
	//initR[2][1] = 0.351598185;
	//initR[2][2] = -0.692512159;

    for(int i = 0; i < MAX_LOOP_COUNT ; i++ ) {
		err = arGetTransMat3( initR, ppos2d, ppos3d, n_pts, aConfig->trans );
		if( err < MAX_FIT_ERROR ) break;
    }

	//aConfig->trans[0][0] = 0.749547308;
	//aConfig->trans[0][1] = 0.041695269;
	//aConfig->trans[0][2] = -0.660636312;

	//aConfig->trans[1][0] = -0.203404048;
	//aConfig->trans[1][1] = -0.935222017;
	//aConfig->trans[1][2] = -0.289804369;

	//aConfig->trans[2][0] = -0.629925096;
	//aConfig->trans[2][1] = 0.351598185;
	//aConfig->trans[2][2] = -0.692512159;

	delete ppos2d;
	delete [] ppos3d;

	if(err > 1e+10)		return(-1); // an actual error has occurred in robustPlanarPose()
	
	aConfig->cVisible = true;
	aConfig->prevF = 1;

	return( err); // NOTE: err is a real number from the interval [0,1e+10]
}



