#pragma once
#include <vector>
#include <opencv2/opencv.hpp>

/*




double initcausal(double *c,int n,double z)
{
	double zk,z2k,iz,sum;
	int k;

	zk = z; iz = 1./z;
	z2k = pow(z,(double)n-1.);
	sum = c[0] + z2k * c[n-1];
	z2k = z2k*z2k*iz;
	for (k=1;k<=n-2;k++) {
		sum += (zk+z2k)*c[k];
		zk *= z;
		z2k *= iz;
	}
	return (sum/(1.-zk*zk));
}

double initanticausal(double *c,int n,double z)
{
	return((z/(z*z-1.))*(z*c[n-2]+c[n-1]));
}

void invspline1D(double *c,int size,double *z,int npoles)
{
	double lambda;
	int n,k;

	/ * normalization * /
	for (k=npoles,lambda=1.;k--;) lambda *= (1.-z[k])*(1.-1./z[k]);
	for (n=size;n--;) c[n] *= lambda;

	/ *----- Loop on poles -----* /
	for (k=0;k<npoles;k++) {

		/ * forward recursion * /
		c[0] = initcausal(c,size,z[k]);
		for (n=1;n<size;n++) 
			c[n] += z[k]*c[n-1];

		/ * backwards recursion * /
		c[size-1] = initanticausal(c,size,z[k]);
		for (n=size-1;n--;) 
			c[n] = z[k]*(c[n+1]-c[n]);

	}
}

/ * coefficients for spline of order >3 * /
void splinen(float *c,float t,float *a,int n)
{
	int i,k;
	float xn;

	memset((void *)c,0,(n+1)*sizeof(float));
	for (k=0;k<=n+1;k++) { 
		xn = ipow(t+(float)k,n);
		for (i=k;i<=n;i++) 
			c[i] += a[i-k]*xn;
	}
}

/ * coefficients for cubic interpolant (Keys' function) * /
void keys(float *c,float t,float a)
{
	float t2,at;

	t2 = t*t;
	at = a*t;
	c[0] = a*t2*(1.0-t);
	c[1] = (2.0*a+3.0 - (a+2.0)*t)*t2 - at;
	c[2] = ((a+2.0)*t - a-3.0)*t2 + 1.0;
	c[3] = a*(t-2.0)*t2 + at;
}

/ * coefficients for cubic spline * /
void spline3(float *c,float t)
{
	float tmp;

	tmp = 1.-t;
	c[0] = 0.1666666666*t*t*t;
	c[1] = 0.6666666666-0.5*tmp*tmp*(1.+t);
	c[2] = 0.6666666666-0.5*t*t*(2.-t);
	c[3] = 0.1666666666*tmp*tmp*tmp;
}

// Guoshen Yu, 2010.09.21, Windows version
//void finvspline(vector<float> &in,int order,vector<float>& out, int width, int height)
void finvspline(cv::Mat img_in, int order, cv::Mat* img_out)
// void finvspline(float *in,int order,float *out, int width, int height)
{
	double *c,*d,z[5];
	int npoles,nx,ny,x,y;

	ny = img_in->cols; 
	nx = img_in->rows;

	/ * initialize poles of associated z-filter * /
	switch (order) 
	{
	case 2: z[0]=-0.17157288;  / * sqrt(8)-3 * /
		break;

	case 3: z[0]=-0.26794919;  / * sqrt(3)-2 * / 
		break;

	case 4: z[0]=-0.361341; z[1]=-0.0137254;
		break;

	case 5: z[0]=-0.430575; z[1]=-0.0430963;
		break;

	case 6: z[0]=-0.488295; z[1]=-0.0816793; z[2]=-0.00141415;
		break;

	case 7: z[0]=-0.53528; z[1]=-0.122555; z[2]=-0.00914869;
		break;

	case 8: z[0]=-0.574687; z[1]=-0.163035; z[2]=-0.0236323; z[3]=-0.000153821;
		break;

	case 9: z[0]=-0.607997; z[1]=-0.201751; z[2]=-0.0432226; z[3]=-0.00212131;
		break;

	case 10: z[0]=-0.636551; z[1]=-0.238183; z[2]=-0.065727; z[3]=-0.00752819;
		z[4]=-0.0000169828;
		break;

	case 11: z[0]=-0.661266; z[1]=-0.27218; z[2]=-0.0897596; z[3]=-0.0166696; 
		z[4]=-0.000510558;
		break;

	default:
		printf("finvspline: order should be in 2..11.\n");
		exit(-1);
	}

	npoles = order/2;

	/ * initialize double array containing image * /
	c = (double *)malloc(nx*ny*sizeof(double));
	d = (double *)malloc(nx*ny*sizeof(double));

	//for (x=nx*ny;x--;)
	//  c[x] = (double)in[x];

	//value copy
	for(int yy=0; yy<ny; yy++){
		for(int xx=0; xx<nx; xx++){
			c[yy*nx + xx] = img_in.at(yy, xx);
		}
	}

	/ * apply filter on lines * /
	for (y=0;y<ny;y++) 
		invspline1D(c+y*nx,nx,z,npoles);

	/ * transpose * /
	for (x=0;x<nx;x++)
		for (y=0;y<ny;y++) 
			d[x*ny+y] = c[y*nx+x];

	/ * apply filter on columns * /
	for (x=0;x<nx;x++) 
		invspline1D(d+x*ny,ny,z,npoles);

	/ * transpose directy into image * /
	for (x=0;x<nx;x++)
		for (y=0;y<ny;y++) 
			//out[y*nx+x] = (float)(d[x*ny+y]);
			img_out->at(y, x) = (float)(d[x*ny+y]));

	/ * free array * /
	free(d);
	free(c);
}


/ * pre-computation for spline of order >3 * /
void init_splinen(float *a,int n)
{
	int k;

	a[0] = 1.;
	for (k=2;k<=n;k++) a[0]/=(float)k;
	for (k=1;k<=n+1;k++)
		a[k] = - a[k-1] *(float)(n+2-k)/(float)k;
}

//void fproj(float *in, float *out, int nx, int ny, int *sx, int *sy, float *bg, int *o, float *p, char *i, float X1, float Y1, float X2, float Y2, float X3, float Y3, float *x4, float *y4)
void fproj(cv::Mat img_in, cv::Mat* img_out, int *sx, int *sy, float *bg, int *o, float *p, char *i, float X1, float Y1, float X2, float Y2, float X3, float Y3, float *x4, float *y4)
/ *     Fimage in,out;
     int    *sx,*sy,*o;
     char   *i;
     float  *bg,*p,X1,Y1,X2,Y2,X3,Y3,*x4,*y4; * /
{
/ *  int    n1,n2,nx,ny,x,y,xi,yi,adr,dx,dy;* /
  int    n1,n2,x,y,xi,yi,adr,dx,dy;
  float  res,xx,yy,xp,yp,ux,uy,a,b,d,fx,fy,x12,x13,y12,y13;
  float  cx[12],cy[12],ak[13];
 / * Fimage ref,coeffs; * /
//  float *ref, *coeffs;
	//vector<float> ref, coeffs;
	cv::Mat ref, coeffs;


  / * CHECK ORDER * /
  if (*o!=0 && *o!=1 && *o!=-3 && 
      *o!=3 && *o!=5 && *o!=7 && *o!=9 && *o!=11)
  / *  mwerror(FATAL,1,"unrecognized interpolation order.\n"); * /
  {	
	  printf("unrecognized interpolation order.\n");
	  exit(-1);
  }

  int nx, ny;		
  nx = img_in->cols;
  ny = img_in->rows;
  
  / * ALLOCATE NEW IMAGE * /
/ *  nx = in->ncol; ny = in->nrow; * /
/ *  out = mw_change_fimage(out,*sy,*sx); 
  if (!out) mwerror(FATAL,1,"not enough memory\n"); * /


	if (*o>=3) {
/ *    coeffs = mw_new_fimage();
    finvspline(in,*o,coeffs); * /
	  
//	  coeffs = new float[nx*ny];
	  
	  //coeffs = vector<float>(nx*ny);
		//coeffs = cvCreateImage(cvSize(nx, ny), IPL_DEPTH_32F, 1);
	  
		finvspline(img_in, *o, &coeffs);
	  
		ref = coeffs;
	
		if(*o>3)
			init_splinen(ak,*o);
		}
		else{
			//coeffs = NULL;
			ref = img_in;
		}


	/ * COMPUTE NEW BASIS * /
	if(i){
		x12 = (X2-X1)/(float)nx;
		y12 = (Y2-Y1)/(float)nx;
		x13 = (X3-X1)/(float)ny;
		y13 = (Y3-Y1)/(float)ny;
	}
	else{
		x12 = (X2-X1)/(float)(*sx);
		y12 = (Y2-Y1)/(float)(*sx);
		x13 = (X3-X1)/(float)(*sy);
		y13 = (Y3-Y1)/(float)(*sy);
	}



  if (y4) { 
    xx=((*x4-X1)*(Y3-Y1)-(*y4-Y1)*(X3-X1))/((X2-X1)*(Y3-Y1)-(Y2-Y1)*(X3-X1));
    yy=((*x4-X1)*(Y2-Y1)-(*y4-Y1)*(X2-X1))/((X3-X1)*(Y2-Y1)-(Y3-Y1)*(X2-X1));
    a = (yy-1.0)/(1.0-xx-yy);
    b = (xx-1.0)/(1.0-xx-yy);
  } 
  else 
    {
      a=b=0.0;
    }

     


  / ********** MAIN LOOP ********** /

  for (x=0;x<*sx;x++) 
    for (y=0;y<*sy;y++) {
      
      / * COMPUTE LOCATION IN INPUT IMAGE * /
      if (i) {
	xx = 0.5+(((float)x-X1)*y13-((float)y-Y1)*x13)/(x12*y13-y12*x13);
	yy = 0.5-(((float)x-X1)*y12-((float)y-Y1)*x12)/(x12*y13-y12*x13);
	d = 1.0-(a/(a+1.0))*xx/(float)nx-(b/(b+1.0))*yy/(float)ny;
	xp = xx/((a+1.0)*d);
	yp = yy/((b+1.0)*d);
      } else {
	fx = (float)x + 0.5;
	fy = (float)y + 0.5;
	d = a*fx/(float)(*sx)+b*fy/(float)(*sy)+1.0;
	xx = (a+1.0)*fx/d;
	yy = (b+1.0)*fy/d;
	xp = X1 + xx*x12 + yy*x13;
	yp = Y1 + xx*y12 + yy*y13;
      }


      / * INTERPOLATION * /
      
      if (*o==0) { 
	
	/ * zero order interpolation (pixel replication) * /
	xi = (int)floor((double)xp); 
	yi = (int)floor((double)yp);
/ *	if (xi<0 || xi>=in->ncol || yi<0 || yi>=in->nrow)* /
	if (xi<0 || xi>=nx || yi<0 || yi>=ny)
	  res = *bg; 
	else 
		/ * res = in->gray[yi*in->ncol+xi]; * /
		res = img_in.at(yi, xi);
      } else { 
	
	/ * higher order interpolations * /
	if (xp<0. || xp>(float)nx || yp<0. || yp>(float)ny) res=*bg; 
	else {
	  xp -= 0.5; yp -= 0.5;
	  xi = (int)floor((double)xp); 
	  yi = (int)floor((double)yp);
	  ux = xp-(float)xi;
	  uy = yp-(float)yi;
	  switch (*o) 
	    {
	    case 1: / * first order interpolation (bilinear) * /
	      n2 = 1;
	      cx[0]=ux;	cx[1]=1.-ux;
	      cy[0]=uy; cy[1]=1.-uy;
	      break;
	      
	    case -3: / * third order interpolation (bicubic Keys' function) * /
	      n2 = 2;
	      keys(cx,ux,*p);
	      keys(cy,uy,*p);
	      break;

	    case 3: / * spline of order 3 * /
	      n2 = 2;
	      spline3(cx,ux);
	      spline3(cy,uy);
	      break;

	    default: / * spline of order >3 * /
	      n2 = (1+*o)/2;
	      splinen(cx,ux,ak,*o);
	      splinen(cy,uy,ak,*o);
	      break;
	    }
	  
	  res = 0.; n1 = 1-n2;
	  / * this test saves computation time * /
	  if (xi+n1>=0 && xi+n2<nx && yi+n1>=0 && yi+n2<ny) {
	    adr = yi*nx+xi; 
	    for (dy=n1;dy<=n2;dy++) 
	      for (dx=n1;dx<=n2;dx++) 
/ *		res += cy[n2-dy]*cx[n2-dx]*ref->gray[adr+nx*dy+dx];* /
			  //res += cy[n2-dy]*cx[n2-dx]* ref[adr+nx*dy+dx];
			  res += cy[n2-dy]*cx[n2-dx]* ref.at(dy, dx+adr);
	  } else 
	    for (dy=n1;dy<=n2;dy++)
	      for (dx=n1;dx<=n2;dx++) 
/ *		res += cy[n2-dy]*cx[n2-dx]*v(ref,xi+dx,yi+dy,*bg); * /
	  res += cy[n2-dy]*cx[n2-dx]*v(ref,xi+dx,yi+dy,*bg,nx,ny); 
	}
      }		
      / * out->gray[y*(*sx)+x] = res; * /
		img_out->at(y, x) = res;
    }
  //if (coeffs) 
	  / * mw_delete_fimage(coeffs); * /
	//  delete[] coeffs; 
}
*/




/*
static void printHomo(cv::Mat homo){

	for(int y=0; y<homo.rows; y++){
		for(int x=0; x<homo.cols; x++){
			printf("%f ", homo.at<float>(y, x));
		}
		printf("\n");
	}
}

void calHomograpy(vector<cv::Mat> &cont_homo, cv::Mat img_in, float tx1, float tx2, float ty1, float ty2)
{
	cv::Mat src(4, 2, CV_32FC1);
	cv::Mat dst(4, 2, CV_32FC1);

	src.at<float>(0,0) = 0;				src.at<float>(0,1) = 0;
	src.at<float>(1,0) = img_in.cols;	src.at<float>(1,1) = 0;
	src.at<float>(2,0) = 0;				src.at<float>(2,1) = img_in.rows;
	src.at<float>(3,0) = img_in.cols;	src.at<float>(3,1) = img_in.rows;

	dst.at<float>(0,0) = img_in.cols/2 - img_in.cols*tx1/2;	dst.at<float>(0,1) = img_in.rows/2 - img_in.rows*ty1/2;
	dst.at<float>(1,0) = img_in.cols/2 + img_in.cols*tx1/2;	dst.at<float>(1,1) = img_in.rows/2 - img_in.rows*ty2/2;
	dst.at<float>(2,0) = img_in.cols/2 - img_in.cols*tx2/2;	dst.at<float>(2,1) = img_in.rows/2 + img_in.rows*ty1/2;
	dst.at<float>(3,0) = img_in.cols/2 + img_in.cols*tx2/2;	dst.at<float>(3,1) = img_in.rows/2 + img_in.rows*ty2/2;

	cv::Mat homo;
	vector<uchar> outlier_mask;
	homo = cv::findHomography(src, dst, outlier_mask, cv::RANSAC, 1);

	cont_homo.push_back(homo);
}

static void tiltImage(cv::Mat imgSrc, cv::Mat &imgDst, cv::Mat homo){

	warpPerspective(imgSrc, imgDst, homo, imgSrc.size());
}


*/