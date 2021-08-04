/*
  Nayeem  - A UCI chess engine Based on Stockfish. Copyright (C) 2013-2021 Mohamed Nayeem
  Family  - Stockfish
  Author  - Mohamed Nayeem
  License - GPL-3.0
*/

// header used in NNUE evaluation function

#ifndef NNUE_EVALUATE_NNUE_H_INCLUDED
#define NNUE_EVALUATE_NNUE_H_INCLUDED

#include "nnue_feature_transformer.h"

#include <memory>

namespace Stockfish::Eval::NNUE {

  // Hash value of evaluation function structure
  constexpr std::uint32_t HashValue =
      FeatureTransformer::get_hash_value() ^ Network::get_hash_value();

  // Deleter for automating release of memory area
  template <typename T>
  struct AlignedDeleter {
    void operator()(T* ptr) const {
      ptr->~T();
      std_aligned_free(ptr);
    }
  };

  template <typename T>
  struct LargePageDeleter {
    void operator()(T* ptr) const {
      ptr->~T();
      aligned_large_pages_free(ptr);
    }
  };

  template <typename T>
  using AlignedPtr = std::unique_ptr<T, AlignedDeleter<T>>;

  template <typename T>
  using LargePagePtr = std::unique_ptr<T, LargePageDeleter<T>>;

}  // namespace Stockfish::Eval::NNUE

#endif // #ifndef NNUE_EVALUATE_NNUE_H_INCLUDED
