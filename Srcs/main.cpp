/*
  Nayeem  - A UCI chess engine. Copyright (C) 2013-2015 Mohamed Nayeem
  Family  - Stockfish
  Author  - Mohamed Nayeem
  License - GPL-3.0
*/

#include <iostream>

#include "bitboard.h"
#include "position.h"
#include "search.h"
#include "thread.h"
#include "tt.h"
#include "uci.h"
#include "syzygy/tbprobe.h"

namespace PSQT {
  void init();
}

int main(int argc, char* argv[]) {

  std::cout << engine_info() << std::endl;

  UCI::init(Options);
  PSQT::init();
  Bitboards::init();
  Position::init();
  Bitbases::init();
  Search::init();
  Pawns::init();
  Threads.init();
  Tablebases::init(Options["SyzygyPath"]);
  TT.resize(Options["Hash"]);

  UCI::loop(argc, argv);

  Threads.exit();
  return 0;
}
