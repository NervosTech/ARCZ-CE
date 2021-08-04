/*
  Nayeem  - A UCI chess engine Based on Stockfish. Copyright (C) 2013-2021 Mohamed Nayeem
  Family  - Stockfish
  Author  - Mohamed Nayeem
  License - GPL-3.0
*/

// Input features and network structure used in NNUE evaluation function

#ifndef NNUE_ARCHITECTURE_H_INCLUDED
#define NNUE_ARCHITECTURE_H_INCLUDED

#include "nnue_common.h"

#include "features/half_ka_v2.h"

#include "layers/input_slice.h"
#include "layers/affine_transform.h"
#include "layers/clipped_relu.h"

namespace Stockfish::Eval::NNUE {

  // Input features used in evaluation function
  using FeatureSet = Features::HalfKAv2;

  // Number of input feature dimensions after conversion
  constexpr IndexType TransformedFeatureDimensions = 512;
  constexpr IndexType PSQTBuckets = 8;
  constexpr IndexType LayerStacks = 8;

  namespace Layers {

    // Define network structure
    using InputLayer = InputSlice<TransformedFeatureDimensions * 2>;
    using HiddenLayer1 = ClippedReLU<AffineTransform<InputLayer, 16>>;
    using HiddenLayer2 = ClippedReLU<AffineTransform<HiddenLayer1, 32>>;
    using OutputLayer = AffineTransform<HiddenLayer2, 1>;

  }  // namespace Layers

  using Network = Layers::OutputLayer;

  static_assert(TransformedFeatureDimensions % MaxSimdWidth == 0, "");
  static_assert(Network::OutputDimensions == 1, "");
  static_assert(std::is_same<Network::OutputType, std::int32_t>::value, "");

}  // namespace Stockfish::Eval::NNUE

#endif // #ifndef NNUE_ARCHITECTURE_H_INCLUDED
