/*
  Nayeem  - A UCI chess engine Based on Stockfish. Copyright (C) 2013-2021 Mohamed Nayeem
  Family  - Stockfish
  Author  - Mohamed Nayeem
  License - GPL-3.0
*/

// Class for difference calculation of NNUE evaluation function

#ifndef NNUE_ACCUMULATOR_H_INCLUDED
#define NNUE_ACCUMULATOR_H_INCLUDED

#include "nnue_architecture.h"

namespace Stockfish::Eval::NNUE {

  // Class that holds the result of affine transformation of input features
  struct alignas(CacheLineSize) Accumulator {
    std::int16_t accumulation[2][TransformedFeatureDimensions];
    std::int32_t psqtAccumulation[2][PSQTBuckets];
    bool computed[2];
  };

}  // namespace Stockfish::Eval::NNUE

#endif // NNUE_ACCUMULATOR_H_INCLUDED
