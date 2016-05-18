/*
  Nayeem - A UCI chess engine derived from Stockfish.
  Copyright (C) 2016 Mohamed Nayeem
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

#include <iostream>

#include "bitboard.h"
#include "evaluate.h"
#include "position.h"
#include "search.h"
#include "thread.h"
#include "tt.h"
#include "uci.h"
#include "syzygy/tbprobe.h"

void SETUP_PRIVILEGES();
void FREE_MEM(void *);

int main(int argc, char* argv[]) {

  std::cout << engine_info() << std::endl;
  #ifndef BENCH
    SETUP_PRIVILEGES();
  #endif
  UCI::init(Options);
  TT.resize(Options["Hash"]);
  Tune::init();
  PSQT::init();
  Bitboards::init();
  Position::init();
  Bitbases::init();
  Search::init();
  Eval::init();
  Pawns::init();
  Threads.init();
  Tablebases::init(Options["SyzygyPath"]);

  UCI::loop(argc, argv);

  if (large_use) {
    FREE_MEM(TT.mem);  
    TT.mem = nullptr;
  }

  Threads.exit();
}
