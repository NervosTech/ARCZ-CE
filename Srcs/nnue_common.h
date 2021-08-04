/*
  Nayeem  - A UCI chess engine Based on Stockfish. Copyright (C) 2013-2021 Mohamed Nayeem
  Family  - Stockfish
  Author  - Mohamed Nayeem
  License - GPL-3.0
*/

// Constants used in NNUE evaluation function

#ifndef NNUE_COMMON_H_INCLUDED
#define NNUE_COMMON_H_INCLUDED

#include <cstring>
#include <iostream>

#include "../misc.h"  // for IsLittleEndian

#if defined(USE_AVX2)
#include <immintrin.h>

#elif defined(USE_SSE41)
#include <smmintrin.h>

#elif defined(USE_SSSE3)
#include <tmmintrin.h>

#elif defined(USE_SSE2)
#include <emmintrin.h>

#elif defined(USE_MMX)
#include <mmintrin.h>

#elif defined(USE_NEON)
#include <arm_neon.h>
#endif

namespace Stockfish::Eval::NNUE {

  // Version of the evaluation file
  constexpr std::uint32_t Version = 0x7AF32F20u;

  // Constant used in evaluation value calculation
  constexpr int OutputScale = 16;
  constexpr int WeightScaleBits = 6;

  // Size of cache line (in bytes)
  constexpr std::size_t CacheLineSize = 64;

  // SIMD width (in bytes)
  #if defined(USE_AVX2)
  constexpr std::size_t SimdWidth = 32;

  #elif defined(USE_SSE2)
  constexpr std::size_t SimdWidth = 16;

  #elif defined(USE_MMX)
  constexpr std::size_t SimdWidth = 8;

  #elif defined(USE_NEON)
  constexpr std::size_t SimdWidth = 16;
  #endif

  constexpr std::size_t MaxSimdWidth = 32;

  // Type of input feature after conversion
  using TransformedFeatureType = std::uint8_t;
  using IndexType = std::uint32_t;

  // Round n up to be a multiple of base
  template <typename IntType>
  constexpr IntType ceil_to_multiple(IntType n, IntType base) {
      return (n + base - 1) / base * base;
  }

  // read_little_endian() is our utility to read an integer (signed or unsigned, any size)
  // from a stream in little-endian order. We swap the byte order after the read if
  // necessary to return a result with the byte ordering of the compiling machine.
  template <typename IntType>
  inline IntType read_little_endian(std::istream& stream) {
      IntType result;

      if (IsLittleEndian)
          stream.read(reinterpret_cast<char*>(&result), sizeof(IntType));
      else
      {
          std::uint8_t u[sizeof(IntType)];
          typename std::make_unsigned<IntType>::type v = 0;

          stream.read(reinterpret_cast<char*>(u), sizeof(IntType));
          for (std::size_t i = 0; i < sizeof(IntType); ++i)
              v = (v << 8) | u[sizeof(IntType) - i - 1];

          std::memcpy(&result, &v, sizeof(IntType));
      }

      return result;
  }

  // write_little_endian() is our utility to write an integer (signed or unsigned, any size)
  // to a stream in little-endian order. We swap the byte order before the write if
  // necessary to always write in little endian order, independantly of the byte
  // ordering of the compiling machine.
  template <typename IntType>
  inline void write_little_endian(std::ostream& stream, IntType value) {

      if (IsLittleEndian)
          stream.write(reinterpret_cast<const char*>(&value), sizeof(IntType));
      else
      {
          std::uint8_t u[sizeof(IntType)];
          typename std::make_unsigned<IntType>::type v = value;

          std::size_t i = 0;
          // if constexpr to silence the warning about shift by 8
          if constexpr (sizeof(IntType) > 1)
          {
            for (; i + 1 < sizeof(IntType); ++i)
            {
                u[i] = v;
                v >>= 8;
            }
          }
          u[i] = v;

          stream.write(reinterpret_cast<char*>(u), sizeof(IntType));
      }
  }

  // read_little_endian(s, out, N) : read integers in bulk from a little indian stream.
  // This reads N integers from stream s and put them in array out.
  template <typename IntType>
  inline void read_little_endian(std::istream& stream, IntType* out, std::size_t count) {
      if (IsLittleEndian)
          stream.read(reinterpret_cast<char*>(out), sizeof(IntType) * count);
      else
          for (std::size_t i = 0; i < count; ++i)
              out[i] = read_little_endian<IntType>(stream);
  }

  // write_little_endian(s, values, N) : write integers in bulk to a little indian stream.
  // This takes N integers from array values and writes them on stream s.
  template <typename IntType>
  inline void write_little_endian(std::ostream& stream, const IntType* values, std::size_t count) {
      if (IsLittleEndian)
          stream.write(reinterpret_cast<const char*>(values), sizeof(IntType) * count);
      else
          for (std::size_t i = 0; i < count; ++i)
              write_little_endian<IntType>(stream, values[i]);
  }

}  // namespace Stockfish::Eval::NNUE

#endif // #ifndef NNUE_COMMON_H_INCLUDED
