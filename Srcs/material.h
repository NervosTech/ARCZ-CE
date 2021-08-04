/*
  Nayeem  - A UCI chess engine Based on Stockfish. Copyright (C) 2013-2021 Mohamed Nayeem
  Family  - Stockfish
  Author  - Mohamed Nayeem
  License - GPL-3.0
*/

#ifndef MATERIAL_H_INCLUDED
#define MATERIAL_H_INCLUDED

#include "endgame.h"
#include "misc.h"
#include "position.h"
#include "types.h"

namespace Stockfish::Material {

/// Material::Entry contains various information about a material configuration.
/// It contains a material imbalance evaluation, a function pointer to a special
/// endgame evaluation function (which in most cases is NULL, meaning that the
/// standard evaluation function will be used), and scale factors.
///
/// The scale factors are used to scale the evaluation score up or down. For
/// instance, in KRB vs KR endgames, the score is scaled down by a factor of 4,
/// which will result in scores of absolute value less than one pawn.

struct Entry {

  Score imbalance() const { return score; }
  Phase game_phase() const { return (Phase)gamePhase; }
  bool specialized_eval_exists() const { return evaluationFunction != nullptr; }
  Value evaluate(const Position& pos) const { return (*evaluationFunction)(pos); }

  // scale_factor() takes a position and a color as input and returns a scale factor
  // for the given color. We have to provide the position in addition to the color
  // because the scale factor may also be a function which should be applied to
  // the position. For instance, in KBP vs K endgames, the scaling function looks
  // for rook pawns and wrong-colored bishops.
  ScaleFactor scale_factor(const Position& pos, Color c) const {
    ScaleFactor sf = scalingFunction[c] ? (*scalingFunction[c])(pos)
                                        :  SCALE_FACTOR_NONE;
    return sf != SCALE_FACTOR_NONE ? sf : ScaleFactor(factor[c]);
  }

  Key key;
  const EndgameBase<Value>* evaluationFunction;
  const EndgameBase<ScaleFactor>* scalingFunction[COLOR_NB]; // Could be one for each
                                                             // side (e.g. KPKP, KBPsK)
  Score score;
  int16_t gamePhase;
  uint8_t factor[COLOR_NB];
};

typedef HashTable<Entry, 8192> Table;

Entry* probe(const Position& pos);

} // namespace Stockfish::Material

#endif // #ifndef MATERIAL_H_INCLUDED
