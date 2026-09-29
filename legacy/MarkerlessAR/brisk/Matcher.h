#pragma once
#include <opencv2/opencv.hpp>
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#include <emmintrin.h>
#include <tmmintrin.h>
#else
#include <arm_neon.h>
typedef int32x4_t __m128i;
#endif
using namespace cv;

namespace BDHamming{

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#if defined(__GNUC__) || defined(__clang__)
	static const char __attribute__((aligned(16))) MASK_4bit[16] = {0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf};
	static const uint8_t __attribute__((aligned(16))) POPCOUNT_4bit[16] = { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4};
	static const __m128i shiftval = _mm_set_epi32 (0,0,0,4);
#endif
#ifdef _MSC_VER
	__declspec(align(16)) static const char MASK_4bit[16] = {0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf};
	__declspec(align(16)) static const uint8_t POPCOUNT_4bit[16] = { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4};
	static const __m128i shiftval = _mm_set_epi32 (0,0,0,4);
#endif
#endif // x86

	//type definition
	//typedef int ResultType;
	//typedef unsigned char ValueType;

	template<unsigned char b>
	struct ByteBits
	{
		/**
		* number of bits in the byte given by the template constant
		*/
		enum
		{
			COUNT = ((b >> 0) & 1) +
			((b >> 1) & 1) +
			((b >> 2) & 1) +
			((b >> 3) & 1) +
			((b >> 4) & 1) +
			((b >> 5) & 1) +
			((b >> 6) & 1) +
			((b >> 7) & 1)
		};
	};

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
	// - SSSE3 - better alorithm, minimized psadbw usage - adapted from http://wm.ite.pl/articles/sse-popcount.html
	__inline__ uint32_t ssse3_popcntofXORed(const __m128i* signature1, const __m128i* signature2, const int numberOf128BitWords)
	{

		uint32_t result = 0;

		register __m128i xmm0;
		register __m128i xmm1;
		register __m128i xmm2;
		register __m128i xmm3;
		register __m128i xmm4;
		register __m128i xmm5;
		register __m128i xmm6;
		register __m128i xmm7;

		xmm7 = _mm_load_si128 ((__m128i *)POPCOUNT_4bit);
		xmm6 = _mm_load_si128 ((__m128i *)MASK_4bit);
		xmm5 = _mm_setzero_si128();

		const size_t end=(size_t)(signature1+numberOf128BitWords);

		xmm4 = xmm5;

		do{
			xmm0 = _mm_xor_si128 ( (__m128i)*signature1++, (__m128i)*signature2++);
			xmm1 = xmm0;
			xmm1 = _mm_srl_epi16 (xmm1, shiftval);
			xmm0 = _mm_and_si128 (xmm0, xmm6);
			xmm1 = _mm_and_si128 (xmm1, xmm6);
			xmm2 = xmm7;
			xmm3 = xmm7;
			xmm2 = _mm_shuffle_epi8(xmm2, xmm0);
			xmm3 = _mm_shuffle_epi8(xmm3, xmm1);
			xmm4 = _mm_add_epi8(xmm4, xmm2);
			xmm4 = _mm_add_epi8(xmm4, xmm3);
		}while((size_t)signature1<end);

		xmm4 = _mm_sad_epu8(xmm4, xmm5);
		xmm5 = _mm_add_epi32(xmm5, xmm4);
		xmm0 = _mm_cvtps_epi32(_mm_movehl_ps(_mm_cvtepi32_ps(xmm0), _mm_cvtepi32_ps(xmm5)));
		xmm0 = _mm_add_epi32(xmm0, xmm5);
		result = _mm_cvtsi128_si32 (xmm0);
		return result;
	}
#else
	// ARM/other: scalar fallback for Hamming distance
	__inline__ uint32_t ssse3_popcntofXORed(const __m128i* signature1, const __m128i* signature2, const int numberOf128BitWords)
	{
		const uint8_t* a = reinterpret_cast<const uint8_t*>(signature1);
		const uint8_t* b = reinterpret_cast<const uint8_t*>(signature2);
		uint32_t result = 0;
		for (int i = 0; i < numberOf128BitWords * 16; i++) {
			result += __builtin_popcount(a[i] ^ b[i]);
		}
		return result;
	}
#endif // x86 vs ARM

	unsigned char byteBitsLookUp(unsigned char b)
	{
		static const unsigned char table[256] =
		{
			ByteBits<0>::COUNT,
			ByteBits<1>::COUNT,
			ByteBits<2>::COUNT,
			ByteBits<3>::COUNT,
			ByteBits<4>::COUNT,
			ByteBits<5>::COUNT,
			ByteBits<6>::COUNT,
			ByteBits<7>::COUNT,
			ByteBits<8>::COUNT,
			ByteBits<9>::COUNT,
			ByteBits<10>::COUNT,
			ByteBits<11>::COUNT,
			ByteBits<12>::COUNT,
			ByteBits<13>::COUNT,
			ByteBits<14>::COUNT,
			ByteBits<15>::COUNT,
			ByteBits<16>::COUNT,
			ByteBits<17>::COUNT,
			ByteBits<18>::COUNT,
			ByteBits<19>::COUNT,
			ByteBits<20>::COUNT,
			ByteBits<21>::COUNT,
			ByteBits<22>::COUNT,
			ByteBits<23>::COUNT,
			ByteBits<24>::COUNT,
			ByteBits<25>::COUNT,
			ByteBits<26>::COUNT,
			ByteBits<27>::COUNT,
			ByteBits<28>::COUNT,
			ByteBits<29>::COUNT,
			ByteBits<30>::COUNT,
			ByteBits<31>::COUNT,
			ByteBits<32>::COUNT,
			ByteBits<33>::COUNT,
			ByteBits<34>::COUNT,
			ByteBits<35>::COUNT,
			ByteBits<36>::COUNT,
			ByteBits<37>::COUNT,
			ByteBits<38>::COUNT,
			ByteBits<39>::COUNT,
			ByteBits<40>::COUNT,
			ByteBits<41>::COUNT,
			ByteBits<42>::COUNT,
			ByteBits<43>::COUNT,
			ByteBits<44>::COUNT,
			ByteBits<45>::COUNT,
			ByteBits<46>::COUNT,
			ByteBits<47>::COUNT,
			ByteBits<48>::COUNT,
			ByteBits<49>::COUNT,
			ByteBits<50>::COUNT,
			ByteBits<51>::COUNT,
			ByteBits<52>::COUNT,
			ByteBits<53>::COUNT,
			ByteBits<54>::COUNT,
			ByteBits<55>::COUNT,
			ByteBits<56>::COUNT,
			ByteBits<57>::COUNT,
			ByteBits<58>::COUNT,
			ByteBits<59>::COUNT,
			ByteBits<60>::COUNT,
			ByteBits<61>::COUNT,
			ByteBits<62>::COUNT,
			ByteBits<63>::COUNT,
			ByteBits<64>::COUNT,
			ByteBits<65>::COUNT,
			ByteBits<66>::COUNT,
			ByteBits<67>::COUNT,
			ByteBits<68>::COUNT,
			ByteBits<69>::COUNT,
			ByteBits<70>::COUNT,
			ByteBits<71>::COUNT,
			ByteBits<72>::COUNT,
			ByteBits<73>::COUNT,
			ByteBits<74>::COUNT,
			ByteBits<75>::COUNT,
			ByteBits<76>::COUNT,
			ByteBits<77>::COUNT,
			ByteBits<78>::COUNT,
			ByteBits<79>::COUNT,
			ByteBits<80>::COUNT,
			ByteBits<81>::COUNT,
			ByteBits<82>::COUNT,
			ByteBits<83>::COUNT,
			ByteBits<84>::COUNT,
			ByteBits<85>::COUNT,
			ByteBits<86>::COUNT,
			ByteBits<87>::COUNT,
			ByteBits<88>::COUNT,
			ByteBits<89>::COUNT,
			ByteBits<90>::COUNT,
			ByteBits<91>::COUNT,
			ByteBits<92>::COUNT,
			ByteBits<93>::COUNT,
			ByteBits<94>::COUNT,
			ByteBits<95>::COUNT,
			ByteBits<96>::COUNT,
			ByteBits<97>::COUNT,
			ByteBits<98>::COUNT,
			ByteBits<99>::COUNT,
			ByteBits<100>::COUNT,
			ByteBits<101>::COUNT,
			ByteBits<102>::COUNT,
			ByteBits<103>::COUNT,
			ByteBits<104>::COUNT,
			ByteBits<105>::COUNT,
			ByteBits<106>::COUNT,
			ByteBits<107>::COUNT,
			ByteBits<108>::COUNT,
			ByteBits<109>::COUNT,
			ByteBits<110>::COUNT,
			ByteBits<111>::COUNT,
			ByteBits<112>::COUNT,
			ByteBits<113>::COUNT,
			ByteBits<114>::COUNT,
			ByteBits<115>::COUNT,
			ByteBits<116>::COUNT,
			ByteBits<117>::COUNT,
			ByteBits<118>::COUNT,
			ByteBits<119>::COUNT,
			ByteBits<120>::COUNT,
			ByteBits<121>::COUNT,
			ByteBits<122>::COUNT,
			ByteBits<123>::COUNT,
			ByteBits<124>::COUNT,
			ByteBits<125>::COUNT,
			ByteBits<126>::COUNT,
			ByteBits<127>::COUNT,
			ByteBits<128>::COUNT,
			ByteBits<129>::COUNT,
			ByteBits<130>::COUNT,
			ByteBits<131>::COUNT,
			ByteBits<132>::COUNT,
			ByteBits<133>::COUNT,
			ByteBits<134>::COUNT,
			ByteBits<135>::COUNT,
			ByteBits<136>::COUNT,
			ByteBits<137>::COUNT,
			ByteBits<138>::COUNT,
			ByteBits<139>::COUNT,
			ByteBits<140>::COUNT,
			ByteBits<141>::COUNT,
			ByteBits<142>::COUNT,
			ByteBits<143>::COUNT,
			ByteBits<144>::COUNT,
			ByteBits<145>::COUNT,
			ByteBits<146>::COUNT,
			ByteBits<147>::COUNT,
			ByteBits<148>::COUNT,
			ByteBits<149>::COUNT,
			ByteBits<150>::COUNT,
			ByteBits<151>::COUNT,
			ByteBits<152>::COUNT,
			ByteBits<153>::COUNT,
			ByteBits<154>::COUNT,
			ByteBits<155>::COUNT,
			ByteBits<156>::COUNT,
			ByteBits<157>::COUNT,
			ByteBits<158>::COUNT,
			ByteBits<159>::COUNT,
			ByteBits<160>::COUNT,
			ByteBits<161>::COUNT,
			ByteBits<162>::COUNT,
			ByteBits<163>::COUNT,
			ByteBits<164>::COUNT,
			ByteBits<165>::COUNT,
			ByteBits<166>::COUNT,
			ByteBits<167>::COUNT,
			ByteBits<168>::COUNT,
			ByteBits<169>::COUNT,
			ByteBits<170>::COUNT,
			ByteBits<171>::COUNT,
			ByteBits<172>::COUNT,
			ByteBits<173>::COUNT,
			ByteBits<174>::COUNT,
			ByteBits<175>::COUNT,
			ByteBits<176>::COUNT,
			ByteBits<177>::COUNT,
			ByteBits<178>::COUNT,
			ByteBits<179>::COUNT,
			ByteBits<180>::COUNT,
			ByteBits<181>::COUNT,
			ByteBits<182>::COUNT,
			ByteBits<183>::COUNT,
			ByteBits<184>::COUNT,
			ByteBits<185>::COUNT,
			ByteBits<186>::COUNT,
			ByteBits<187>::COUNT,
			ByteBits<188>::COUNT,
			ByteBits<189>::COUNT,
			ByteBits<190>::COUNT,
			ByteBits<191>::COUNT,
			ByteBits<192>::COUNT,
			ByteBits<193>::COUNT,
			ByteBits<194>::COUNT,
			ByteBits<195>::COUNT,
			ByteBits<196>::COUNT,
			ByteBits<197>::COUNT,
			ByteBits<198>::COUNT,
			ByteBits<199>::COUNT,
			ByteBits<200>::COUNT,
			ByteBits<201>::COUNT,
			ByteBits<202>::COUNT,
			ByteBits<203>::COUNT,
			ByteBits<204>::COUNT,
			ByteBits<205>::COUNT,
			ByteBits<206>::COUNT,
			ByteBits<207>::COUNT,
			ByteBits<208>::COUNT,
			ByteBits<209>::COUNT,
			ByteBits<210>::COUNT,
			ByteBits<211>::COUNT,
			ByteBits<212>::COUNT,
			ByteBits<213>::COUNT,
			ByteBits<214>::COUNT,
			ByteBits<215>::COUNT,
			ByteBits<216>::COUNT,
			ByteBits<217>::COUNT,
			ByteBits<218>::COUNT,
			ByteBits<219>::COUNT,
			ByteBits<220>::COUNT,
			ByteBits<221>::COUNT,
			ByteBits<222>::COUNT,
			ByteBits<223>::COUNT,
			ByteBits<224>::COUNT,
			ByteBits<225>::COUNT,
			ByteBits<226>::COUNT,
			ByteBits<227>::COUNT,
			ByteBits<228>::COUNT,
			ByteBits<229>::COUNT,
			ByteBits<230>::COUNT,
			ByteBits<231>::COUNT,
			ByteBits<232>::COUNT,
			ByteBits<233>::COUNT,
			ByteBits<234>::COUNT,
			ByteBits<235>::COUNT,
			ByteBits<236>::COUNT,
			ByteBits<237>::COUNT,
			ByteBits<238>::COUNT,
			ByteBits<239>::COUNT,
			ByteBits<240>::COUNT,
			ByteBits<241>::COUNT,
			ByteBits<242>::COUNT,
			ByteBits<243>::COUNT,
			ByteBits<244>::COUNT,
			ByteBits<245>::COUNT,
			ByteBits<246>::COUNT,
			ByteBits<247>::COUNT,
			ByteBits<248>::COUNT,
			ByteBits<249>::COUNT,
			ByteBits<250>::COUNT,
			ByteBits<251>::COUNT,
			ByteBits<252>::COUNT,
			ByteBits<253>::COUNT,
			ByteBits<254>::COUNT,
			ByteBits<255>::COUNT
		};

		return table[b];
	}

	// this will count the bits in a ^ b
	int distanceSSE3(const unsigned char* a, const unsigned char* b, const int size)
	{
		return ssse3_popcntofXORed(
			(const __m128i*)(a),
			(const __m128i*)(b),
			size/16);
	}

	int distance( const unsigned char* a, const unsigned char* b, int size )
	{
		int result = 0;
		for (int i = 0; i < size; i++)
		{
			result += byteBitsLookUp(a[i] ^ b[i]);
		}
		return result;
	}

	void commonRadiusMatch( const Mat& queryDescriptors, const Mat& trainDescriptors, vector<vector<DMatch> >& matches, float maxDistance )
	{
		int dimension = queryDescriptors.cols;
		matches.reserve(queryDescriptors.rows);

		size_t imgCount = 1;
		for( int qIdx = 0; qIdx < queryDescriptors.rows; qIdx++ )
		{
			matches.push_back( vector<DMatch>() );

			vector<vector<DMatch> >::reverse_iterator curMatches = matches.rbegin();
			for( size_t iIdx = 0; iIdx < imgCount; iIdx++ )
			{
				const unsigned char* d1 = (const unsigned char*)(queryDescriptors.data + queryDescriptors.step*qIdx);
				for( int tIdx = 0; tIdx < trainDescriptors.rows; tIdx++ )
				{
					const unsigned char* d2 = (const unsigned char*)(trainDescriptors.data + trainDescriptors.step*tIdx);
					
					//int d = distance(d1, d2, dimension);
					int d = distanceSSE3(d1, d2, dimension);

					if( d < maxDistance )
						curMatches->push_back( DMatch( qIdx, tIdx, (int)iIdx, (float)d ) );					
				}
			}
			std::sort( curMatches->begin(), curMatches->end() );

		}
	}
};