/*
  Nayeem  - A UCI chess engine Based on Stockfish. Copyright (C) 2013-2021 Mohamed Nayeem
  Family  - Stockfish
  Author  - Mohamed Nayeem
  License - GPL-3.0
*/

#ifndef TIMEMAN_H_INCLUDED
#define TIMEMAN_H_INCLUDED

#include "misc.h"
#include "search.h"
#include "thread.h"

namespace Stockfish {

/// The TimeManagement class computes the optimal time to think depending on
/// the maximum available time, the game move number and other parameters.

class TimeManagement {
public:
  void init(Search::LimitsType& limits, Color us, int ply);
  TimePoint optimum() const { return optimumTime; }
  TimePoint maximum() const { return maximumTime; }
  TimePoint elapsed() const { return Search::Limits.npmsec ?
                                     TimePoint(Threads.nodes_searched()) : now() - startTime; }

  int64_t availableNodes; // When in 'nodes as time' mode

private:
  TimePoint startTime;
  TimePoint optimumTime;
  TimePoint maximumTime;
};

extern TimeManagement Time;

} // namespace Stockfish

#endif // #ifndef TIMEMAN_H_INCLUDED
