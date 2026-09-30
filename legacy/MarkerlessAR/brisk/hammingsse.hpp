/*
    BRISK - Binary Robust Invariant Scalable Keypoints
    Reference implementation of
    [1] Stefan Leutenegger,Margarita Chli and Roland Siegwart, BRISK:
    	Binary Robust Invariant Scalable Keypoints, in Proceedings of
    	the IEEE International Conference on Computer Vision (ICCV2011).

    Copyright (C) 2011  The Autonomous Systems Lab, ETH Zurich,
    Stefan Leutenegger and Margarita Chli.

    This file is part of BRISK.

    BRISK is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    BRISK is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with BRISK.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef HAMMINGSSE_HPP_
#define HAMMINGSSE_HPP_

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
// x86/x64: use SSE/SSSE3 intrinsics
#include <emmintrin.h>
#include <tmmintrin.h>

namespace cv{

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

__inline__ // - SSSE3 - better alorithm, minimized psadbw usage - adapted from http://wm.ite.pl/articles/sse-popcount.html
uint32_t HammingSse::ssse3_popcntofXORed(const __m128i* signature1, const __m128i* signature2, const int numberOf128BitWords) {

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
#include <arm_neon.h>

namespace cv{

// Scalar popcount-of-XORed implementation for non-x86 platforms
__inline__
uint32_t HammingSse::ssse3_popcntofXORed(const __m128i* signature1, const __m128i* signature2, const int numberOf128BitWords) {
	const uint8_t* a = reinterpret_cast<const uint8_t*>(signature1);
	const uint8_t* b = reinterpret_cast<const uint8_t*>(signature2);
	uint32_t result = 0;
	for (int i = 0; i < numberOf128BitWords * 16; i++) {
		uint8_t v = a[i] ^ b[i];
		// Count bits using lookup or __builtin_popcount
		result += __builtin_popcount(v);
	}
	return result;
}

#endif // x86 vs ARM

}
#endif /* HAMMINGSSE_HPP_ */
