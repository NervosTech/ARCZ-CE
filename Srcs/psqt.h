/*
  Nayeem  - A UCI chess engine Based on Stockfish. Copyright (C) 2013-2021 Mohamed Nayeem
  Family  - Stockfish
  Author  - Mohamed Nayeem
  License - GPL-3.0
*/


#ifndef PSQT_H_INCLUDED
#define PSQT_H_INCLUDED


#include "types.h"


namespace Stockfish::PSQT
{

extern Score psq[PIECE_NB][SQUARE_NB];

// Fill psqt array from a set of internally linked parameters
extern void init();

} // namespace Stockfish::PSQT


#endif // PSQT_H_INCLUDED
