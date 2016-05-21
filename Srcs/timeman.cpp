/*
  Nayeem - A UCI chess engine. Copyright (C) 2013-2015 Mohamed Nayeem
  Nayeem is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  Nayeem is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with Nayeem. If not, see <http://www.gnu.org/licenses/>.
*/

#include <algorithm>
#include <cfloat>
#include <cmath>

#include "search.h"
#include "timeman.h"
#include "uci.h"

TimeManagement Time; // Our global time management object

namespace {

  enum TimeType { OptimumTime, MaxTime };

  template<TimeType T>
  int remaining(int myTime, int myInc, int moveOverhead, int ply, int movesToGo)
  {
    double TStealRatio;

    if (movesToGo)
    {
        if (ply < 81) TStealRatio = (T == OptimumTime ? 0.0224 : 0.112) * exp(-ply / 300.0);
        else 
            TStealRatio = (T == OptimumTime ? 0.018 : 0.09);
        TStealRatio *= 36.0 / movesToGo; 
    }
    else
    {
        int hply = (ply + 1) / 2;
        TStealRatio = (T == OptimumTime ? 0.018 : 0.09) * (1.0 + 15.0 * hply / (500.0 + hply));
    }
    
    double ratio = std::min(1.0, TStealRatio * (1.0 + 66 * myInc / double(myInc + 2 * myTime)));
    int hypMyTime = std::max(0, myTime - moveOverhead);

    return int(hypMyTime * ratio); // Intel C++ asks for an explicit cast
  }

} // namespace


/// init() is called at the beginning of the search and calculates the allowed
/// thinking time out of the time control and current game ply. We support four
/// different kinds of time controls, passed in 'limits':
///
///  inc == 0 && movestogo == 0 means: x basetime  [sudden death!]
///  inc == 0 && movestogo != 0 means: x moves in y minutes
///  inc >  0 && movestogo == 0 means: x basetime + z increment
///  inc >  0 && movestogo != 0 means: x moves in y minutes + z increment

void TimeManagement::init(Search::LimitsType& limits, Color us, int ply)
{
  int moveOverhead    = Options["Move Overhead"];
  int npmsec          = Options["nodestime"];

  // If we have to play in 'nodes as time' mode, then convert from time
  // to nodes, and use resulting values in time management formulas.
  // WARNING: Given npms (nodes per millisecond) must be much lower then
  // the real engine speed to avoid time losses.
  if (npmsec)
  {
      if (!availableNodes) // Only once at game start
          availableNodes = npmsec * limits.time[us]; // Time is in msec

      // Convert from millisecs to nodes
      limits.time[us] = (int)availableNodes;
      limits.inc[us] *= npmsec;
      limits.npmsec = npmsec;
  }

  startTime = limits.startTime;

      optimumTime = remaining<OptimumTime>(limits.time[us], limits.inc[us], moveOverhead, ply, limits.movestogo);
	  maximumTime = remaining<MaxTime    >(limits.time[us], limits.inc[us], moveOverhead, ply, limits.movestogo);

  if (Options["Ponder"])
      optimumTime += optimumTime / 4;
}
