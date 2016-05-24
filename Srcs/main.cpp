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

#include <iostream>

#include "bitboard.h"
#include "evaluate.h"
#include "position.h"
#include "search.h"
#include "thread.h"
#include "tt.h"
#include "uci.h"

#ifdef SYZYGY_TB
#include "syzygy/tbprobe.h"
#endif

#ifdef LOMONOSOV_TB
#include "lomonosov_probe.h"
#endif

int main(int argc, char* argv[]) {

  std::cout << engine_info() << std::endl;

  UCI::init(Options);
  Tune::init();
  PSQT::init();
  Bitboards::init();
  Position::init();
  Bitbases::init();
  Search::init();
  Eval::init();
  Pawns::init();
  Threads.init();
  TT.resize(Options["Hash"]);

#ifdef SYZYGY_TB
  Tablebases::init(Options["SyzygyPath"]);
#endif

#ifdef LOMONOSOV_TB
  //init Lomonosov TB
  int result = lomonosov_change_server_mode(Options["Lomonosov Server Mode"], Options["Lomonosov Server Console"]);
  sync_cout << "Lomonosov tables are" << (result == -1 ? " not" : "") << " loaded" << sync_endl;
#endif

  UCI::loop(argc, argv);

  Threads.exit();
  return 0;
}
