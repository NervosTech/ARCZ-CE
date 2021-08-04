/*
  Nayeem  - A UCI chess engine Based on Stockfish. Copyright (C) 2013-2021 Mohamed Nayeem
  Family  - Stockfish
  Author  - Mohamed Nayeem
  License - GPL-3.0
*/

#ifndef EVALUATE_H_INCLUDED
#define EVALUATE_H_INCLUDED

#include <string>
#include <optional>

#include "types.h"

namespace Stockfish {

class Position;

namespace Eval {

  std::string trace(Position& pos);
  Value evaluate(const Position& pos);

  extern bool useNNUE;
  extern std::string eval_file_loaded;

  // The default net name MUST follow the format nn-[SHA256 first 12 digits].nnue
  // for the build process (profile-build and fishtest) to work. Do not change the
  // name of the macro, as it is used in the Makefile.
  #define EvalFileDefaultName   "nn-56a5f1c4173a.nnue"

  namespace NNUE {

    std::string trace(Position& pos);
    Value evaluate(const Position& pos, bool adjusted = false);

    void init();
    void verify();

    bool load_eval(std::string name, std::istream& stream);
    bool save_eval(std::ostream& stream);
    bool save_eval(const std::optional<std::string>& filename);

  } // namespace NNUE

} // namespace Eval

} // namespace Stockfish

#endif // #ifndef EVALUATE_H_INCLUDED
