/*
  Copyright 2010 Computer Vision Lab,
  Ecole Polytechnique Federale de Lausanne (EPFL), Switzerland.
  All rights reserved.

  Author: Michael Calonder (http://cvlab.epfl.ch/~calonder)
  Version: 1.0

  This file is part of the BRIEF DEMO software.

  BRIEF DEMO is free software; you can redistribute it and/or modify it under the
  terms of the GNU General Public License as published by the Free Software
  Foundation; either version 2 of the License, or (at your option) any later
  version.

  BRIEF DEMO is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
  PARTICULAR PURPOSE. See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along with
  BRIEF DEMO; if not, write to the Free Software Foundation, Inc., 51 Franklin
  Street, Fifth Floor, Boston, MA 02110-1301, USA
*/

#pragma once

#include <fstream>

#include <time.h>
#include <windows.h> //I've ommited this line.
#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif

#ifndef __MCUTILS_DEFINES
#define __MCUTILS_DEFINES

// Warnings and errors
#define STDOUT_WARNING(msg) utils::stdoutWarning(msg, __FILE__, __LINE__, __FUNCTION__)
#define STDOUT_WARNING_NOHALT(msg) utils::stdoutWarning(msg, __FILE__, __LINE__, __FUNCTION__, false)
#define STDOUT_ERROR(msg) utils::stdoutError(msg, __FILE__, __LINE__, __FUNCTION__)

// 'ASSURE' expresses a hard constraint, even enforced in release code; remember
// to switch off for profiling
#define ASSURE(cond) if (!(cond)) STDOUT_ERROR("Assure failed: '" + std::string(#cond) + "'")
#define ASSURE_FEX(url)\
   if (!utils::fileExists(url))\
      STDOUT_ERROR("File not found (variable '" + std::string(#url) + "') at\n '" + std::string(url) + "'");
#define ASSURE_EQ(lhs,rhs)\
   if (lhs != rhs)\
      STDOUT_ERROR("`==` assertion violated\n" + std::string(#lhs) + "==" + utils::numToStr(lhs) +\
                   " == " + std::string(#rhs) + "==" + utils::numToStr(rhs));
#define ASSURE_NEQ(lhs,rhs)\
   if (lhs == rhs)\
      STDOUT_ERROR("`!=` assertion violated\n" + std::string(#lhs) + "==" + utils::numToStr(lhs) +\
                   " != " + std::string(#rhs) + "==" + utils::numToStr(rhs));
#define ASSURE_GEQ(lhs,rhs)\
   if (lhs < rhs)\
      STDOUT_ERROR("`>=` assertion violated\n" + std::string(#lhs) + "==" + utils::numToStr(lhs) +\
                   " >= " + std::string(#rhs) + "==" + utils::numToStr(rhs));
#define ASSURE_GT(lhs,rhs)\
   if (lhs <= rhs)\
      STDOUT_ERROR("`>` assertion violated\n" + std::string(#lhs) + "==" + utils::numToStr(lhs) +\
                   " > " + std::string(#rhs) + "==" + utils::numToStr(rhs));
#define ASSURE_LEQ(lhs,rhs)\
   if (lhs > rhs)\
      STDOUT_ERROR("`<=` assertion violated\n" + std::string(#lhs) + "==" + utils::numToStr(lhs) +\
                   " <= " + std::string(#rhs) + "==" + utils::numToStr(rhs));
#define ASSURE_LT(lhs,rhs)\
   if (lhs >= rhs)\
      STDOUT_ERROR("`<` assertion violated\n" + std::string(#lhs) + "==" + utils::numToStr(lhs) +\
                   " < " + std::string(#rhs) + "==" + utils::numToStr(rhs));

// Rounds up to next even number
#define CEIL_EVEN(v) (int(v) + (v-int(v)!=0) + ((int(v) + (v-int(v)!=0))%2>0))

// Rounds to next odd number towards +/-INF
#define ROUND_ODD(v) (int((v)/2.)*2 + ((v)>=0) - ((v)<0))

// Integer to std::string
#define intToStr(a) numToStr((a), "%i")

#define ToRadian(degree)    ((degree)*(PI/180.0f))
#define ToDegree(radian)    ((radian)*(180.0f/PI))

// OpenCV colors
// OpenCV colors
#define RED       CV_RGB(255,0,0)
#define GREEN     CV_RGB(0,255,0)
#define BLUE      CV_RGB(0,0,255)
#define BLACK     CV_RGB(0,0,0)
#define WHITE     CV_RGB(255,255,255)
#define GRAY      CV_RGB(190,190,190)
#define DKGRAY    CV_RGB(84,84,84)
#define LTGRAY    CV_RGB(211,211,211)
#define SILVER    CV_RGB(192,192,192)
#define LTBLUE    CV_RGB(135,206,235)
#define DKBLUE    CV_RGB(0,0,128)
#define LTRED     CV_RGB(255,69,0)
#define DKRED     CV_RGB(139,0,0)
#define LTGREEN   CV_RGB(0,255,127)
#define DKGREEN   CV_RGB(0,100,0)
#define ORANGE    CV_RGB(255,127,0)
#define YELLOW    CV_RGB(255,255,0)
#define MAGENTA   CV_RGB(139,0,139)
#define VIOLET    CV_RGB(79,47,79)

// Square of an expression. Careful with things like SQR(a++)!
#define SQR(a) ((a)*(a))
#endif

typedef signed char int8_t; 
typedef signed short int16_t; 
typedef signed long int32_t; 
typedef unsigned char uint8_t; 
typedef unsigned short uint16_t; 
typedef unsigned long uint32_t;

namespace utils {

bool        fileExists(const std::string& file);
char        showInWindow(const std::string win_name, const IplImage *image, int wait=-1, void(*mouse_cb_func)(int,int,int,int,void*)=NULL, bool destroy_win=true);
void        stdoutError(std::string msg, const char *file, int line, const char *func);
IplImage*   stackImagesVertically(const IplImage *top, const IplImage *bottom, const bool big_in_color);
void        stdoutWarning(std::string msg, const char *file, int line, const char *func, bool pause=true);
double      randNormal(const double std, const double mu=0.);
double      randUniform();    // \in [0,1)
static int			gettimeofday(struct timeval *tv, struct timezone *tz);

// numToStr traits
template< typename T > struct numToStr_trait {};
template<> struct numToStr_trait<int> { static inline std::string getFormat() { return "%i"; } };
template<> struct numToStr_trait<float> { static inline std::string getFormat() { return "%.10e"; } };
template<> struct numToStr_trait<double> { static inline std::string getFormat() { return "%.15e"; } };
template<> struct numToStr_trait<char> { static inline std::string getFormat() { return "%i"; } };
template<> struct numToStr_trait<signed char> { static inline std::string getFormat() { return "%i"; } };
template<> struct numToStr_trait<unsigned char> { static inline std::string getFormat() { return "%i"; } };
template<> struct numToStr_trait<unsigned int> { static inline std::string getFormat() { return "%u"; } };
template<> struct numToStr_trait<unsigned long int> { static inline std::string getFormat() { return "%lu"; } };
template<> struct numToStr_trait<unsigned long long> { static inline std::string getFormat() { return "%lu"; } };
template<> struct numToStr_trait<uint16_t> { static inline std::string getFormat() { return "%u"; } };
template<> struct numToStr_trait<void*> { static inline std::string getFormat() { return "%p"; } };
template<> struct numToStr_trait<IplImage*> { static inline std::string getFormat() { return "%p"; } };
template<> struct numToStr_trait<const IplImage*> { static inline std::string getFormat() { return "%p"; } };

struct timezone 
{
	int  tz_minuteswest; /* minutes W of Greenwich */
	int  tz_dsttime;     /* type of dst correction */
};

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
	FILETIME ft;
	unsigned __int64 tmpres = 0;
	static int tzflag;

	if (NULL != tv)
	{
		GetSystemTimeAsFileTime(&ft);

		tmpres |= ft.dwHighDateTime;
		tmpres <<= 32;
		tmpres |= ft.dwLowDateTime;

		/*converting file time to unix epoch*/
		tmpres -= DELTA_EPOCH_IN_MICROSECS; 
		tmpres /= 10;  /*convert into microseconds*/
		tv->tv_sec = (long)(tmpres / 1000000UL);
		tv->tv_usec = (long)(tmpres % 1000000UL);
	}

	if (NULL != tz)
	{
		if (!tzflag)
		{
			_tzset();
			tzflag++;
		}
		tz->tz_minuteswest = _timezone / 60;
		tz->tz_dsttime = _daylight;
	}

	return 0;
}


template< typename T >
std::string numToStr(T a, std::string format="")
{
   char buf[128];
   if (format.empty()) format = numToStr_trait< T >::getFormat();

   sprintf(buf, format.c_str(), a);
   return std::string(buf);
}


// Local storage container, implements Singleton pattern
struct LocalStorage {

   static LocalStorage& getInst() {
      static bool first = true;
      if (first) {
         first = false;
         LocalStorage::instance = new LocalStorage();
      }
      return *instance;
   }

   CvRNG rng_obj;               // PRNG object

protected:
   LocalStorage() {
      rng_obj = cvRNG(23);
   }

   static LocalStorage* instance;
};


inline double getMs()
{
   struct timeval t0;
   gettimeofday(&t0, NULL);
   double ret = t0.tv_sec * 1000.0;
   ret += ((double) t0.tv_usec)*0.001;
   return ret;
}


/** Minimum in vector. Optionally returns the index of the minimum. Valid for
  * instantiations of any numeric type
**/
template< typename T >
T minVect(const T *v, int n, int *ind=NULL)
{
   assert(n > 0);

   T min = v[0];
   if (ind != NULL) *ind = 0;
   for (int i=1; i<n; i++)
      if (v[i] < min) {
         min = v[i];
         if (ind != NULL) *ind=i;
      }

   return min;
}


/** Reads a rectangular matrix of arbitrary type from a file. No dimension
  * checks. Output matrix is allocated if NULL.
**/

template<typename T>
T* readMat(const std::string& name, T* &mat, int m, int n)
{
   ASSURE_FEX(name);
   std::ifstream ifs(name.c_str(), std::ios_base::in);
   if (!ifs.is_open()) STDOUT_WARNING("read_mat: cannot read '"+name+"'.");

   if (mat == NULL) mat = new T[m*n];
   for (int i=0; i<m*n; i++) {
      if (ifs.eof()) STDOUT_ERROR("Attempt to read beyond EOF in '" + name + "'.");
      ifs >> mat[i];
   }

   return mat;
}

template<typename T>
void writeMat(const std::string& name, T* mat, int m, int n, std::string fmt="", bool append=false)
{
   if (fmt.empty()) fmt = numToStr_trait< T >::getFormat();

   std::ofstream ofs(name.c_str(), (append ? std::ios::app : std::ios::out));
   if (!ofs.good()) {
      STDOUT_WARNING("Cannot open '" + name + "' for output.");
      return;
   }

   char buf[1024];
   std::string tmp;
   for (int i=0; i<m; i++) {
      tmp = "";
      for (int j=0; j<n; j++) {
         sprintf(buf, fmt.c_str(), mat[i*n+j]);
         tmp += std::string(buf) + " ";
      }
      ofs << tmp.substr(0, tmp.length()-1) << std::endl;
   }

   ofs.close();
}

template< typename T >
static inline T* getRow(const IplImage *image, int y)
{
   return ((T*) ((image)->imageData + (y) * (image)->widthStep));
}

template< typename T >
void computeIntegralImage(const IplImage * I, T* data)
{
   assert(I->depth == (int)IPL_DEPTH_8U || I->depth == (int)IPL_DEPTH_8S);

   {
      const uchar* rowI = getRow< uchar >(I, 0);
      T* rowII = data;
      T sum = 0;
      for (int u = 0; u < I->width; ++u)
      {
         sum += rowI[u];
         rowII[u] = sum;
      }
   }

   for (int v = 1; v < I->height; ++v)
   {
      uchar *rowI = getRow< uchar >(I, v);
      T* rowII = data + v*I->width;
      T* prowII = rowII - I->width;
      T sum = 0;
      for (int u = 0; u < I->width; u++)
      {
         sum += rowI[u];
         rowII[u] = sum + prowII[u];
      }
   }
}


}  // namespace mcu
