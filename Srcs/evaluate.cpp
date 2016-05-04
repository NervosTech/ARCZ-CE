/*
  Nayeem , a UCI chess playing engine derived from Stockfish
  

  Nayeem  is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Nayeem  is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <algorithm>
#include <cassert>
#include <cstring>   // For std::memset
#include <iomanip>
#include <sstream>

#include "bitboard.h"
#include "evaluate.h"
#include "material.h"
#include "pawns.h"
#include "uci.h"

namespace {

  namespace Tracing {

    enum Term { // First 8 entries are for PieceType
      MATERIAL = 8, IMBALANCE, THREAT, PASSED, SPACE, TOTAL, TERM_NB
    };

    Score scores[COLOR_NB][TERM_NB];
    std::ostream& operator<<(std::ostream& os, Term idx);



    double to_cp(Value v);
    void write(int idx, Color c, Score s);
    void write(int idx, Score w, Score b = SCORE_ZERO);
    std::string do_trace(const Position& pos);
  }


  // Struct EvalInfo contains various information computed and collected
  // by the evaluation functions.
  struct EvalInfo {

    // Pointers to material and pawn hash table entries
    Material::Entry* mi;
    Pawns::Entry* pi;

    // attackedBy[color][piece type] is a bitboard representing all squares
    // attacked by a given color and piece type, attackedBy[color][ALL_PIECES]
    // contains all squares attacked by the given color.
    Bitboard attackedBy[COLOR_NB][PIECE_TYPE_NB];

    // kingRing[color] is the zone around the king which is considered
    // by the king safety evaluation. This consists of the squares directly
    // adjacent to the king, and the three (or two, for a king on an edge file)
    // squares two ranks in front of the king. For instance, if black's king
    // is on g8, kingRing[BLACK] is a bitboard containing the squares f8, h8,
    // f7, g7, h7, f6, g6 and h6.
    Bitboard kingRing[COLOR_NB];

    // kingAttackersCount[color] is the number of pieces of the given color
    // which attack a square in the kingRing of the enemy king.
    int kingAttackersCount[COLOR_NB];

    // kingAttackersWeight[color] is the sum of the "weight" of the pieces of the
    // given color which attack a square in the kingRing of the enemy king. The
    // weights of the individual piece types are given by the elements in the
    // KingAttackWeights array.
    int kingAttackersWeight[COLOR_NB];

    // kingAdjacentZoneAttacksCount[color] is the number of attacks by the given
    // color to squares directly adjacent to the enemy king. Pieces which attack
    // more than one square are counted multiple times. For instance, if there is
    // a white knight on g5 and black's king is on g8, this white knight adds 2
    // to kingAdjacentZoneAttacksCount[WHITE].
    int kingAdjacentZoneAttacksCount[COLOR_NB];

    Bitboard pinnedPieces[COLOR_NB];
  };


  // Evaluation weights, initialized from UCI options
  enum { PawnStructure, PassedPawns, Space, KingSafety };
  struct Weight { int mg, eg; } Weights[4];



  #define V(v) Value(v)
  #define S(mg, eg) make_score(mg, eg)

  // Internal evaluation weights. These are applied on top of the evaluation
  // weights read from UCI parameters. The purpose is to be able to change
  // the evaluation weights while keeping the default values of the UCI
  // parameters at 100, which looks prettier.
  //
  // Values modified by Joona Kiiski
  const Score WeightsInternal[] = {
    S(214, 203), S(193, 262), S(47, 0), S(330, 0)
  };
  // MobilityBonus[PieceType][attacked] contains bonuses for middle and end
  // game, indexed by piece type and number of attacked squares in the MobilityArea.
  const Score MobilityBonus[][32] = {
    {}, {},
    { S(-75,-76), S(-56,-54), S(- 9,-26), S( -2,-10), S(  6,  5), S( 15, 11), // Knights
      S( 22, 26), S( 30, 28), S( 36, 29) },
    { S(-48,-58), S(-21,-19), S( 16, -2), S( 26, 12), S( 37, 22), S( 51, 42), // Bishops
      S( 54, 54), S( 63, 58), S( 65, 63), S( 71, 70), S( 79, 74), S( 81, 86),
      S( 92, 90), S( 97, 94) },
    { S(-56,-78), S(-25,-18), S(-11, 26), S( -5, 55), S( -4, 70), S( -1, 81), // Rooks
      S(  8,109), S( 14,120), S( 21,128), S( 23,143), S( 31,154), S( 32,160),
      S( 43,165), S( 49,168), S( 59,169) },
    { S(-40,-35), S(-25,-12), S(  2,  7), S(  4, 19), S( 14, 37), S( 24, 55), // Queens
      S( 25, 62), S( 40, 76), S( 43, 79), S( 47, 87), S( 54, 94), S( 56,102),
      S( 60,111), S( 70,116), S( 72,118), S( 73,122), S( 75,128), S( 77,130),
      S( 85,133), S( 94,136), S( 99,140), S(108,157), S(112,158), S(113,161),
      S(118,174), S(119,177), S(123,191), S(128,199) }
  };

  // Mask of allowed outpost squares indexed by color
  const Bitboard OutpostMask[COLOR_NB] = {
    Rank4BB | Rank5BB | Rank6BB, Rank5BB | Rank4BB | Rank3BB
  };

  // Outpost[knight/bishop][supported by pawn] contains bonuses for knights and
  // bishops outposts, bigger if outpost piece is supported by a pawn.
  const Score Outpost[][2] = {
    { S(43,11), S(65,20) }, // Knights
    { S(20, 3), S(29, 8) }  // Bishops
  };

  // ReachableOutpost[knight/bishop][supported by pawn] contains bonuses for knights and
  // bishops which can reach a outpost square in one move, bigger if outpost square is supported by a pawn.
  const Score ReachableOutpost[][2] = {
    { S(21, 5), S(35, 8) }, // Knights
    { S( 8, 0), S(14, 4) }  // Bishops
  };

  // Threat[minor/rook][attacked PieceType] contains
  // bonuses according to which piece type attacks which one.
  // Attacks on lesser pieces which are pawn defended are not considered.
  const Score Threat[2][PIECE_TYPE_NB] = {
{ S(0, 0), S(0, 29), S(45, 50), S(46, 50), S(74, 111), S(46,116) }, // Minor attacks
{ S(0, 0), S(0, 22), S(43, 60), S(45, 57), S( 0,  32), S(34, 51) }  // Rook attacks
  };

  // ThreatenedByPawn[PieceType] contains a penalty according to which piece
  // type is attacked by a pawn.
  const Score ThreatenedByPawn[PIECE_TYPE_NB] = {
    S(0, 0), S(0, 0), S(179, 132), S(128, 130), S(218, 209), S(209, 211)
  };

  // Passed[mg/eg][rank] contains midgame and endgame bonuses for passed pawns.
  // We don't use a Score because we process the two components independently.
  const Value Passed[][RANK_NB] = {
    { V(5), V( 5), V(31), V(73), V(166), V(252) },
    { V(7), V(14), V(38), V(73), V(166), V(252) }
  };

  // PassedFile[File] contains a bonus according to the file of a passed pawn.
  const Score PassedFile[] = {
    S( 12,  10), S( 3, 10), S( 1, -8), S(-27, -12),
    S(-27, -12), S( 1, -8), S( 3, 10), S( 12,  10)
  };

  const Score ThreatenedByHangingPawn = S(80, 62);

  // Assorted bonuses and penalties used by evaluation
  const Score KingOnOne          = S( 3, 66);
  const Score KingOnMany         = S( 8,133);
  const Score Tempo              = S(24, 11); 
  const Score RookOnPawn         = S( 8, 24);
  const Score RookOnOpenFile     = S(43, 21);
  const Score RookOnSemiOpenFile = S(19, 10);
  const Score BishopPawns        = S( 8, 12);
  const Score MinorBehindPawn    = S(16,  0);
  const Score TrappedRook        = S(92,  0);
  const Score LooseEnemies       = S( 0, 25);
  const Score Unstoppable        = S( 0, 20);
        Score Hanging            = S(48, 27);
  const Score PawnAttackThreat   = S(38, 22);
  const Score Checked            = S(20, 20);

  // Penalty for a bishop on a1/h1 (a8/h8 for black) which is trapped by
  // a friendly pawn on b2/g2 (b7/g7 for black). This can obviously only
  // happen in Chess960 games.
  const Score TrappedBishopA1H1 = S(50, 50);

  #undef S
  #undef V

  // SpaceMask[Color] contains the area of the board which is considered
  // by the space evaluation. In the middlegame, each side is given a bonus
  // based on how many squares inside this area are safe and available for
  // friendly minor pieces.
  const Bitboard SpaceMask[COLOR_NB] = {
    (FileCBB | FileDBB | FileEBB | FileFBB) & (Rank2BB | Rank3BB | Rank4BB),
    (FileCBB | FileDBB | FileEBB | FileFBB) & (Rank7BB | Rank6BB | Rank5BB)
  };

  // King danger constants and variables. The king danger scores are looked-up
  // in KingDanger[]. Various little "meta-bonuses" measuring the strength
  // of the enemy attack are added up into an integer, which is used as an
  // index to KingDanger[].
  Score KingDanger[512];

  // KingAttackWeights[PieceType] contains king attack weights by piece type
  const int KingAttackWeights[PIECE_TYPE_NB] = { 0, 0, 7, 5, 4, 1 };

  // Penalties for enemy's safe checks
  const int QueenContactCheck = 89;
  const int QueenCheck        = 52;
  const int RookCheck         = 45;
  const int BishopCheck       = 5;
  const int KnightCheck       = 17;

  // KingDanger[attackUnits] contains the actual king danger weighted
  // scores, indexed by a calculated integer number.


  // apply_weight() weighs score 's' by weight 'w' trying to prevent overflow
  Score apply_weight(Score s, const Weight& w) {
    return make_score(mg_value(s) * w.mg / 256, eg_value(s) * w.eg / 256);
  }


  // weight_option() computes the value of an evaluation weight, by combining
  // two UCI-configurable weights (midgame and endgame) with an internal weight.

  Weight weight_option(const std::string& mgOpt, const std::string& egOpt, Score internalWeight) {

    Weight w = { Options[mgOpt] * mg_value(internalWeight) / 100,
                 Options[egOpt] * eg_value(internalWeight) / 100 };
    return w;
  }


  // init_eval_info() initializes king bitboards for given color adding
  // pawn attacks. To be done at the beginning of the evaluation.

  template<Color Us>
  void init_eval_info(const Position& pos, EvalInfo& ei) {

    const Color  Them = (Us == WHITE ? BLACK   : WHITE);
    const Square Down = (Us == WHITE ? DELTA_S : DELTA_N);

    ei.pinnedPieces[Us] = pos.pinned_pieces(Us);
    ei.attackedBy[Us][ALL_PIECES] = ei.attackedBy[Us][PAWN] = ei.pi->pawn_attacks(Us);
    Bitboard b = ei.attackedBy[Them][KING] = pos.attacks_from<KING>(pos.square<KING>(Them));

    // Init king safety tables only if we are going to use them
    if (pos.non_pawn_material(Us) >= QueenValueMg)
    {
        ei.kingRing[Them] = b | shift_bb<Down>(b);
        b &= ei.attackedBy[Us][PAWN];
        ei.kingAttackersCount[Us] = popcount(b);
        ei.kingAdjacentZoneAttacksCount[Us] = ei.kingAttackersWeight[Us] = 0;
    }
    else
        ei.kingRing[Them] = ei.kingAttackersCount[Us] = 0;
  }


  // evaluate_pieces() assigns bonuses and penalties to the pieces of a given color

  template<PieceType Pt, Color Us, bool Trace>
  Score evaluate_pieces(const Position& pos, EvalInfo& ei, Score* mobility, Bitboard* mobilityArea) {

    Bitboard b, bb;
    Square s;
    Score score = SCORE_ZERO;

    const PieceType NextPt = (Us == WHITE ? Pt : PieceType(Pt + 1));
    const Color Them = (Us == WHITE ? BLACK : WHITE);
    const Square* pl = pos.squares<Pt>(Us);

    ei.attackedBy[Us][Pt] = 0;

    while ((s = *pl++) != SQ_NONE)
    {
        // Find attacked squares, including x-ray attacks for bishops and rooks
        b = Pt == BISHOP ? attacks_bb<BISHOP>(s, pos.pieces() ^ pos.pieces(Us, QUEEN))
          : Pt ==   ROOK ? attacks_bb<  ROOK>(s, pos.pieces() ^ pos.pieces(Us, ROOK, QUEEN))
                         : pos.attacks_from<Pt>(s);

        if (ei.pinnedPieces[Us] & s)
            b &= LineBB[pos.square<KING>(Us)][s];

        ei.attackedBy[Us][ALL_PIECES] |= ei.attackedBy[Us][Pt] |= b;

        if (b & ei.kingRing[Them])
        {
            ei.kingAttackersCount[Us]++;
            ei.kingAttackersWeight[Us] += KingAttackWeights[Pt];
            ei.kingAdjacentZoneAttacksCount[Us] += popcount(b & ei.attackedBy[Them][KING]);
        }

        if (Pt == QUEEN)
            b &= ~(  ei.attackedBy[Them][KNIGHT]
                   | ei.attackedBy[Them][BISHOP]
                   | ei.attackedBy[Them][ROOK]);

        int mob = popcount(b & mobilityArea[Us]);

        mobility[Us] += MobilityBonus[Pt][mob];

        if (Pt == BISHOP || Pt == KNIGHT)
        {
            // Bonus for outpost squares
            bb = OutpostMask[Us] & ~ei.pi->pawn_attacks_span(Them);
            if (bb & s)
                score += Outpost[Pt == BISHOP][!!(ei.attackedBy[Us][PAWN] & s)];
            else
            {
                bb &= b & ~pos.pieces(Us);
                if (bb)
                   score += ReachableOutpost[Pt == BISHOP][!!(ei.attackedBy[Us][PAWN] & bb)];
            }

            // Bonus when behind a pawn
            if (    relative_rank(Us, s) < RANK_5
                && (pos.pieces(PAWN) & (s + pawn_push(Us))))
                score += MinorBehindPawn;

            // Penalty for pawns on same color square of bishop
            if (Pt == BISHOP)
                score -= BishopPawns * ei.pi->pawns_on_same_color_squares(Us, s);

            // An important Chess960 pattern: A cornered bishop blocked by a friendly
            // pawn diagonally in front of it is a very serious problem, especially
            // when that pawn is also blocked.
            if (   Pt == BISHOP
                && pos.is_chess960()
                && (s == relative_square(Us, SQ_A1) || s == relative_square(Us, SQ_H1)))
            {
                Square d = pawn_push(Us) + (file_of(s) == FILE_A ? DELTA_E : DELTA_W);
                if (pos.piece_on(s + d) == make_piece(Us, PAWN))
                    score -= !pos.empty(s + d + pawn_push(Us))                ? TrappedBishopA1H1 * 4
                            : pos.piece_on(s + d + d) == make_piece(Us, PAWN) ? TrappedBishopA1H1 * 2
                                                                              : TrappedBishopA1H1;
            }
        }

        if (Pt == ROOK)
        {
            // Bonus for aligning with enemy pawns on the same rank/file
            if (relative_rank(Us, s) >= RANK_5)
                score += RookOnPawn * popcount(pos.pieces(Them, PAWN) & PseudoAttacks[ROOK][s]);

            // Bonus when on an open or semi-open file
            if (ei.pi->semiopen_file(Us, file_of(s)))
                score += ei.pi->semiopen_file(Them, file_of(s)) ? RookOnOpenFile : RookOnSemiOpenFile;

            // Penalize when trapped by the king, even more if king cannot castle
            if (mob <= 3 && !ei.pi->semiopen_file(Us, file_of(s)))
            {
                Square ksq = pos.square<KING>(Us);

                if (   ((file_of(ksq) < FILE_E) == (file_of(s) < file_of(ksq)))
                    && (rank_of(ksq) == rank_of(s) || relative_rank(Us, ksq) == RANK_1)
                    && !ei.pi->semiopen_side(Us, file_of(ksq), file_of(s) < file_of(ksq)))
                    score -= (TrappedRook - make_score(mob * 22, 0)) * (1 + !pos.can_castle(Us));
            }
        }
    }

    if (Trace)
        Tracing::write(Pt, Us, score);

    // Recursively call evaluate_pieces() of next piece type until KING excluded
    return score - evaluate_pieces<NextPt, Them, Trace>(pos, ei, mobility, mobilityArea);
  }

  template<>
  Score evaluate_pieces<KING, WHITE, false>(const Position&, EvalInfo&, Score*, Bitboard*) { return SCORE_ZERO; }
  template<>
  Score evaluate_pieces<KING, WHITE,  true>(const Position&, EvalInfo&, Score*, Bitboard*) { return SCORE_ZERO; }


  // evaluate_king() assigns bonuses and penalties to a king of a given color

  template<Color Us, bool Trace>
  Score evaluate_king(const Position& pos, const EvalInfo& ei) {

    const Color Them = (Us == WHITE ? BLACK : WHITE);

    Bitboard undefended, b, b1, b2, safe;
    int attackUnits;
    const Square ksq = pos.square<KING>(Us);

    // King shelter and enemy pawns storm
    Score score = ei.pi->king_safety<Us>(pos, ksq);

    // Main king safety evaluation
    if (ei.kingAttackersCount[Them])
    {
        // Find the attacked squares which are defended only by the king...
        undefended =  ei.attackedBy[Them][ALL_PIECES]
                    & ei.attackedBy[Us][KING]
                    & ~(  ei.attackedBy[Us][PAWN]   | ei.attackedBy[Us][KNIGHT]
                        | ei.attackedBy[Us][BISHOP] | ei.attackedBy[Us][ROOK]
                        | ei.attackedBy[Us][QUEEN]);
						
		// ... and those which are not defended at all in the larger king ring
		b =   ei.attackedBy[Them][ALL_PIECES] & ~ei.attackedBy[Us][ALL_PIECES] 
		    & ei.kingRing[Us] & ~pos.pieces(Them);

        // Initialize the 'attackUnits' variable, which is used later on as an
        // index into the KingDanger[] array. The initial value is based on the
        // number and types of the enemy's attacking pieces, the number of
        // attacked and undefended squares around our king and the quality of
        // the pawn shelter (current 'score' value).
        attackUnits =  std::min(72, ei.kingAttackersCount[Them] * ei.kingAttackersWeight[Them])
                     +  9 * ei.kingAdjacentZoneAttacksCount[Them]
                     + 27 * popcount(undefended)
                     + 11 * (popcount(b) + !!ei.pinnedPieces[Us])
                     - 64 * !pos.count<QUEEN>(Them)
                     - mg_value(score) / 8;
					 - eg_value(score) / 10;

        // Analyse the enemy's safe queen contact checks. Firstly, find the
        // undefended squares around the king reachable by the enemy queen...
        b = undefended & ei.attackedBy[Them][QUEEN] & ~pos.pieces(Them);
        if (b)
        {
            // ...and then remove squares not supported by another enemy piece
            b &=  ei.attackedBy[Them][PAWN]   | ei.attackedBy[Them][KNIGHT]
                | ei.attackedBy[Them][BISHOP] | ei.attackedBy[Them][ROOK]
                | ei.attackedBy[Them][KING];

            if (b)attackUnits += QueenContactCheck * popcount(b);
        }

        // Analyse the enemy's safe distance checks for sliders and knights
        safe = ~(ei.attackedBy[Us][ALL_PIECES] | pos.pieces(Them));

        b1 = pos.attacks_from<ROOK  >(ksq) & safe;
        b2 = pos.attacks_from<BISHOP>(ksq) & safe;

        // Enemy queen safe checks
        if ((b1 | b2) & ei.attackedBy[Them][QUEEN])
            attackUnits += QueenCheck, score -= Checked;

        // Enemy rooks safe checks
        if (b1 & ei.attackedBy[Them][ROOK])
            attackUnits += RookCheck, score -= Checked;

        // Enemy bishops safe checks
        if (b2 & ei.attackedBy[Them][BISHOP])
            attackUnits += BishopCheck, score -= Checked;

        // Enemy knights safe checks
        if (pos.attacks_from<KNIGHT>(ksq) & ei.attackedBy[Them][KNIGHT] & safe)
            attackUnits += KnightCheck, score -= Checked;

        // Finally, extract the king danger score from the KingDanger[]
        // array and subtract the score from the evaluation.
        score -= KingDanger[attackUnits];
    }

    if (Trace)
        Tracing::write(KING, Us, score);

    return score;
  }


  // evaluate_threats() assigns bonuses according to the types of the attacking
  // and the type of attacked one.

  template<Color Us, bool Trace>
  Score evaluate_threats(const Position& pos, const EvalInfo& ei) {

    const Color Them        = (Us == WHITE ? BLACK    : WHITE);
    const Square Up         = (Us == WHITE ? DELTA_N  : DELTA_S);
    const Square Left       = (Us == WHITE ? DELTA_NW : DELTA_SE);
    const Square Right      = (Us == WHITE ? DELTA_NE : DELTA_SW);
    const Bitboard TRank2BB = (Us == WHITE ? Rank2BB  : Rank7BB);
    const Bitboard TRank7BB = (Us == WHITE ? Rank7BB  : Rank2BB);

    enum { Minor, Rook };

    Bitboard b, weak, defended, safeThreats;
    Score score = SCORE_ZERO;

    // Small bonus if the opponent has loose pawns or pieces
    if (   (pos.pieces(Them) ^ pos.pieces(Them, QUEEN, KING))
        & ~(ei.attackedBy[Us][ALL_PIECES] | ei.attackedBy[Them][ALL_PIECES]))
        score += LooseEnemies;

    // Non-pawn enemies attacked by a pawn
    weak = (pos.pieces(Them) ^ pos.pieces(Them, PAWN)) & ei.attackedBy[Us][PAWN];

    if (weak)
    {
        b = pos.pieces(Us, PAWN) & ( ~ei.attackedBy[Them][ALL_PIECES]
                                    | ei.attackedBy[Us][ALL_PIECES]);

        safeThreats = (shift_bb<Right>(b) | shift_bb<Left>(b)) & weak;

        if (weak ^ safeThreats)
            score += ThreatenedByHangingPawn;

        while (safeThreats)
            score += ThreatenedByPawn[type_of(pos.piece_on(pop_lsb(&safeThreats)))];
    }

    // Non-pawn enemies defended by a pawn
    defended =  (pos.pieces(Them) ^ pos.pieces(Them, PAWN))
              &  ei.attackedBy[Them][PAWN];

    // Enemies not defended by a pawn and under our attack
    weak =   pos.pieces(Them)
          & ~ei.attackedBy[Them][PAWN]
          &  ei.attackedBy[Us][ALL_PIECES];

    // Add a bonus according to the kind of attacking pieces
    if (defended | weak)
    {
        b = (defended | weak) & (ei.attackedBy[Us][KNIGHT] | ei.attackedBy[Us][BISHOP]);
        while (b)
            score += Threat[Minor][type_of(pos.piece_on(pop_lsb(&b)))];

        b = (pos.pieces(Them, QUEEN) | weak) & ei.attackedBy[Us][ROOK];
        while (b)
            score += Threat[Rook ][type_of(pos.piece_on(pop_lsb(&b)))];

        score += Hanging * popcount(weak & ~ei.attackedBy[Them][ALL_PIECES]);

        b = weak & ei.attackedBy[Us][KING];
        if (b)
            score += more_than_one(b) ? KingOnMany : KingOnOne;
    }

    // Bonus if some pawns can safely push and attack an enemy piece
    b = pos.pieces(Us, PAWN) & ~TRank7BB;
    b = shift_bb<Up>(b | (shift_bb<Up>(b & TRank2BB) & ~pos.pieces()));

    b &=  ~pos.pieces()
        & ~ei.attackedBy[Them][PAWN]
        & (ei.attackedBy[Us][ALL_PIECES] | ~ei.attackedBy[Them][ALL_PIECES]);

    b =  (shift_bb<Left>(b) | shift_bb<Right>(b))
       &  pos.pieces(Them)
       & ~ei.attackedBy[Us][PAWN];

    if (b)
        score += popcount(b) * PawnAttackThreat;

    if (Trace)
        Tracing::write(Tracing::THREAT, Us, score);

    return score;
  }


  // evaluate_passed_pawns() evaluates the passed pawns of the given color

  template<Color Us, bool Trace>
  Score evaluate_passed_pawns(const Position& pos, const EvalInfo& ei) {

    const Color Them = (Us == WHITE ? BLACK : WHITE);

    Bitboard b, squaresToQueen, defendedSquares, unsafeSquares;
    Score score = SCORE_ZERO;

    b = ei.pi->passed_pawns(Us);

    while (b)
    {
        Square s = pop_lsb(&b);

        assert(pos.pawn_passed(Us, s));

        int r = relative_rank(Us, s) - RANK_2;
        int rr = r * (r - 1);

        Value mbonus = Passed[MG][r], ebonus = Passed[EG][r];

        if (rr)
        {
            Square blockSq = s + pawn_push(Us);

            // Adjust bonus based on the king's proximity
            ebonus +=  distance(pos.square<KING>(Them), blockSq) * 5 * rr
                     - distance(pos.square<KING>(Us  ), blockSq) * 2 * rr;

            // If blockSq is not the queening square then consider also a second push
            if (relative_rank(Us, blockSq) != RANK_8)
                ebonus -= distance(pos.square<KING>(Us), blockSq + pawn_push(Us)) * rr;

            // If the pawn is free to advance, then increase the bonus
            if (pos.empty(blockSq))
            {
                // If there is a rook or queen attacking/defending the pawn from behind,
                // consider all the squaresToQueen. Otherwise consider only the squares
                // in the pawn's path attacked or occupied by the enemy.
                defendedSquares = unsafeSquares = squaresToQueen = forward_bb(Us, s);

                Bitboard bb = forward_bb(Them, s) & pos.pieces(ROOK, QUEEN) & pos.attacks_from<ROOK>(s);

                if (!(pos.pieces(Us) & bb))
                    defendedSquares &= ei.attackedBy[Us][ALL_PIECES];

                if (!(pos.pieces(Them) & bb))
                    unsafeSquares &= ei.attackedBy[Them][ALL_PIECES] | pos.pieces(Them);

                // If there aren't any enemy attacks, assign a big bonus. Otherwise
                // assign a smaller bonus if the block square isn't attacked.
                int k = !unsafeSquares ? 18 : !(unsafeSquares & blockSq) ? 8 : 0;

                // If the path to queen is fully defended, assign a big bonus.
                // Otherwise assign a smaller bonus if the block square is defended.
                if (defendedSquares == squaresToQueen)
                    k += 6;

                else if (defendedSquares & blockSq)
                    k += 4;

                mbonus += k * rr, ebonus += k * rr;
            }
            else if (pos.pieces(Us) & blockSq)
                mbonus += rr + r * 2, ebonus += rr + r * 2;
        } // rr != 0

        score += make_score(mbonus, ebonus) + PassedFile[file_of(s)];
    }

    if (Trace)
        Tracing::write(Tracing::PASSED, Us, apply_weight(score, Weights[PassedPawns]));

    // Add the scores to the middlegame and endgame eval
    return apply_weight(score, Weights[PassedPawns]);
  }


  // evaluate_space() computes the space evaluation for a given side. The
  // space evaluation is a simple bonus based on the number of safe squares
  // available for minor pieces on the central four files on ranks 2--4. Safe
  // squares one, two or three squares behind a friendly pawn are counted
  // twice. Finally, the space bonus is multiplied by a weight. The aim is to
  // improve play on game opening.
  template<Color Us>
  Score evaluate_space(const Position& pos, const EvalInfo& ei) {

    const Color Them = (Us == WHITE ? BLACK : WHITE);
    // Find the safe squares for our pieces inside the area defined by
    // SpaceMask[]. A square is unsafe if it is attacked by an enemy
    // pawn, or if it is undefended and attacked by an enemy piece.
    Bitboard safe =   SpaceMask[Us]
                   & ~pos.pieces(Us, PAWN)
                   & ~ei.attackedBy[Them][PAWN]
                   & (ei.attackedBy[Us][ALL_PIECES] | ~ei.attackedBy[Them][ALL_PIECES]);

    // Find all squares which are at most three squares behind some friendly pawn
    Bitboard behind = pos.pieces(Us, PAWN);
    behind |= (Us == WHITE ? behind >>  8 : behind <<  8);
    behind |= (Us == WHITE ? behind >> 16 : behind << 16);

    // Since SpaceMask[Us] is fully on our half of the board
    assert(unsigned(safe >> (Us == WHITE ? 32 : 0)) == 0);

    // ...count safe + (behind & safe) with a single popcount
    int bonus = popcount((Us == WHITE ? safe << 32 : safe >> 32) | (behind & safe));
    int weight =  pos.count<KNIGHT>(Us) + pos.count<BISHOP>(Us)
                + pos.count<KNIGHT>(Them) + pos.count<BISHOP>(Them);

    return make_score(bonus * weight * weight, 0);
  }


  // evaluate_initiative() computes the initiative correction value for the
  // position, i.e. second order bonus/malus based on the known attacking/defending
  // status of the players.
  Score evaluate_initiative(const Position& pos, int asymmetry, Value eg) {

    int kingDistance =  distance<File>(pos.square<KING>(WHITE), pos.square<KING>(BLACK))
                       - distance<Rank>(pos.square<KING>(WHITE), pos.square<KING>(BLACK));
    int pawns = pos.count<PAWN>(WHITE) + pos.count<PAWN>(BLACK);

    // Compute the initiative bonus for the attacking side
    int initiative = 8 * (asymmetry + kingDistance - 15) + 12 * pawns;

    // Now apply the bonus: note that we find the attacking side by extracting
    // the sign of the endgame value, and that we carefully cap the bonus so
    // that the endgame score will never be divided by more than two.
    int value = ((eg > 0) - (eg < 0)) * std::max(initiative, -abs(eg / 2));
    
    return make_score(0, value);
  }

} // namespace
  // do_evaluate() is the evaluation entry point, called directly from evaluate()

  template<bool Trace>
  Value do_evaluate(const Position& pos) {

    assert(!pos.checkers());

    EvalInfo ei;
    Score score, mobility[COLOR_NB] = { SCORE_ZERO, SCORE_ZERO };

    // Initialize score by reading the incrementally updated scores included
    // in the position object (material + piece square tables) and adding a
    // Tempo bonus. Score is computed from the point of view of white.
    score = pos.psq_score() + (pos.side_to_move() == WHITE ? Tempo : -Tempo);

    // Probe the material hash table
    ei.mi = Material::probe(pos);
    score += ei.mi->imbalance();

    // If we have a specialized evaluation function for the current material
    // configuration, call it and return.
    if (ei.mi->specialized_eval_exists())
        return ei.mi->evaluate(pos);

    // Probe the pawn hash table
    ei.pi = Pawns::probe(pos);
    score += apply_weight(ei.pi->pawns_score(), Weights[PawnStructure]);

    // Initialize attack and king safety bitboards
    init_eval_info<WHITE>(pos, ei);
    init_eval_info<BLACK>(pos, ei);

    ei.attackedBy[WHITE][ALL_PIECES] |= ei.attackedBy[WHITE][KING];
    ei.attackedBy[BLACK][ALL_PIECES] |= ei.attackedBy[BLACK][KING];

    // Pawns blocked or on ranks 2 and 3. Will be excluded from the mobility area
    Bitboard blockedPawns[] = {
      pos.pieces(WHITE, PAWN) & (shift_bb<DELTA_S>(pos.pieces()) | Rank2BB | Rank3BB),
      pos.pieces(BLACK, PAWN) & (shift_bb<DELTA_N>(pos.pieces()) | Rank7BB | Rank6BB)
    };

    // Do not include in mobility squares protected by enemy pawns, or occupied
    // by our blocked pawns or king.
    Bitboard mobilityArea[] = {
      ~(ei.attackedBy[BLACK][PAWN] | blockedPawns[WHITE] | pos.square<KING>(WHITE)),
      ~(ei.attackedBy[WHITE][PAWN] | blockedPawns[BLACK] | pos.square<KING>(BLACK))
    };

    // Evaluate pieces and mobility
    score += evaluate_pieces<KNIGHT, WHITE, Trace>(pos, ei, mobility, mobilityArea);
    score += mobility[WHITE] - mobility[BLACK];

    // Evaluate kings after all other pieces because we need complete attack
    // information when computing the king safety evaluation.
    score +=  evaluate_king<WHITE, Trace>(pos, ei)
            - evaluate_king<BLACK, Trace>(pos, ei);

    // Evaluate tactical threats, we need full attack information including king
    score +=  evaluate_threats<WHITE, Trace>(pos, ei)
            - evaluate_threats<BLACK, Trace>(pos, ei);

    // Evaluate passed pawns, we need full attack information including king
    score +=  evaluate_passed_pawns<WHITE, Trace>(pos, ei)
            - evaluate_passed_pawns<BLACK, Trace>(pos, ei);

    // If both sides have only pawns, score for potential unstoppable pawns
    if (!pos.non_pawn_material(WHITE) && !pos.non_pawn_material(BLACK))
    {
        Bitboard b;
        if ((b = ei.pi->passed_pawns(WHITE)) != 0)
            score += int(relative_rank(WHITE, frontmost_sq(WHITE, b))) * Unstoppable;


        if ((b = ei.pi->passed_pawns(BLACK)) != 0)
            score -= int(relative_rank(BLACK, frontmost_sq(BLACK, b))) * Unstoppable;
    }

    // Evaluate space for both sides, only during opening
  if (pos.non_pawn_material(WHITE) + pos.non_pawn_material(BLACK) >= 12222)
    {
        Score s = evaluate_space<WHITE>(pos, ei) - evaluate_space<BLACK>(pos, ei);
        score += apply_weight(s, Weights[Space]);
    }
	

  // Evaluate position potential for the winning side
  score += evaluate_initiative(pos, ei.pi->pawn_asymmetry(), eg_value(score));

    // Scale winning side if position is more drawish than it appears
    Color strongSide = eg_value(score) > VALUE_DRAW ? WHITE : BLACK;
    ScaleFactor sf = ei.mi->scale_factor(pos, strongSide);

    // If we don't already have an unusual scale factor, check for certain
    // types of endgames, and use a lower scale for those.
    if (    ei.mi->game_phase() < PHASE_MIDGAME
        && (sf == SCALE_FACTOR_NORMAL || sf == SCALE_FACTOR_ONEPAWN))
    {
        if (pos.opposite_bishops())
        {
            // Endgame with opposite-colored bishops and no other pieces (ignoring pawns)
            // is almost a draw, in case of KBP vs KB is even more a draw.
            if (   pos.non_pawn_material(WHITE) == BishopValueMg
                && pos.non_pawn_material(BLACK) == BishopValueMg)
                sf = more_than_one(pos.pieces(PAWN)) ? ScaleFactor(31) : ScaleFactor(9);

            // Endgame with opposite-colored bishops, but also other pieces. Still
            // a bit drawish, but not as drawish as with only the two bishops.
            else
                 sf = ScaleFactor(46 * sf / SCALE_FACTOR_NORMAL);
        }
        // Endings where weaker side can place his king in front of the opponent's
        // pawns are drawish.
        else if (    abs(eg_value(score)) <= BishopValueEg
                 &&  ei.pi->pawn_span(strongSide) <= 1
                 && !pos.pawn_passed(~strongSide, pos.square<KING>(~strongSide)))
                 sf = ei.pi->pawn_span(strongSide) ? ScaleFactor(51) : ScaleFactor(37);
    }



    // Interpolate between a middlegame and a (scaled by 'sf') endgame score
    Value v =  mg_value(score) * int(ei.mi->game_phase())
             + eg_value(score) * int(PHASE_MIDGAME - ei.mi->game_phase()) * sf / SCALE_FACTOR_NORMAL;

  v /= int(PHASE_MIDGAME);

  // Keep more pawns when attacking
  int x = pos.count<PAWN>(WHITE) + pos.count<PAWN>(BLACK);
  int malus = (50 * (14 - x)) / 14;
  if (v > VALUE_DRAW)
      v = std::max(v - malus, v / 2);
  else if (v < VALUE_DRAW)
      v = std::min(v + malus, v / 2);

    // In case of tracing add all single evaluation terms for both white and black
    if (Trace)
    {
        Tracing::write(Tracing::MATERIAL, pos.psq_score());
        Tracing::write(Tracing::IMBALANCE, ei.mi->imbalance());
        Tracing::write(PAWN, ei.pi->pawns_score());
        Tracing::write(Tracing::SPACE, apply_weight(evaluate_space<WHITE>(pos, ei), Weights[Space])
                                     , apply_weight(evaluate_space<BLACK>(pos, ei), Weights[Space]));
        Tracing::write(Tracing::TOTAL, score);
    }

    return (pos.side_to_move() == WHITE ? v : -v) + Eval::Tempo;
  }





  // Tracing functions

  double Tracing::to_cp(Value v) { return double(v) / PawnValueEg; }

  void Tracing::write(int idx, Color c, Score s) { scores[c][idx] = s; }

  void Tracing::write(int idx, Score w, Score b) {
    scores[WHITE][idx] = w, scores[BLACK][idx] = b;
  }

  std::ostream& Tracing::operator<<(std::ostream& os, Term t) {

    double wScore[] = { to_cp(mg_value(scores[WHITE][t])), to_cp(eg_value(scores[WHITE][t])) };
    double bScore[] = { to_cp(mg_value(scores[BLACK][t])), to_cp(eg_value(scores[BLACK][t])) };

    if (t == MATERIAL || t == IMBALANCE || t == Term(PAWN) || t == TOTAL)
        os << "  ---   --- |   ---   --- | ";
    else
        os << std::setw(5) << wScore[MG] << " " << std::setw(5) << wScore[EG] << " | "
           << std::setw(5) << bScore[MG] << " " << std::setw(5) << bScore[EG] << " | ";

    os << std::setw(5) << wScore[MG] - bScore[MG] << " "
       << std::setw(5) << wScore[EG] - bScore[EG] << " \n";

    return os;
  }

  std::string Tracing::do_trace(const Position& pos) {

    std::memset(scores, 0, sizeof(scores));

    Value v = do_evaluate<true>(pos);
    v = pos.side_to_move() == WHITE ? v : -v; // White's point of view

    std::stringstream ss;
    ss << std::showpoint << std::noshowpos << std::fixed << std::setprecision(2)
       << "      Eval term |    White    |    Black    |    Total    \n"
       << "                |   MG    EG  |   MG    EG  |   MG    EG  \n"
       << "----------------+-------------+-------------+-------------\n"
       << "       Material | " << Term(MATERIAL)
       << "      Imbalance | " << Term(IMBALANCE)
       << "          Pawns | " << Term(PAWN)
       << "        Knights | " << Term(KNIGHT)
       << "         Bishop | " << Term(BISHOP)
       << "          Rooks | " << Term(ROOK)
       << "         Queens | " << Term(QUEEN)
       << "    King safety | " << Term(KING)
       << "        Threats | " << Term(THREAT)
       << "   Passed pawns | " << Term(PASSED)
       << "          Space | " << Term(SPACE)
       << "----------------+-------------+-------------+-------------\n"
       << "          Total | " << Term(TOTAL);

    ss << "\nTotal Evaluation: " << to_cp(v) << " (white side)\n";

    return ss.str();
  }



namespace Eval {

  // Init spsa params
  void init_params() {
	  
    // Hanging
    Hanging = make_score(int(Options["Hanging (Midgame)"]) , int(Options["Hanging (Endgame)"]));
  }

  /// evaluate() is the main evaluation function. It returns a static evaluation
  /// of the position always from the point of view of the side to move.

  Value evaluate(const Position& pos) {
    return do_evaluate<false>(pos);
  }



  /// trace() is like evaluate(), but instead of returning a value, it returns
  /// a string (suitable for outputting to stdout) that contains the detailed
  /// descriptions and values of each evaluation term. It's mainly used for
  /// debugging.
  std::string trace(const Position& pos) {
    return Tracing::do_trace(pos);
  }


  /// init() computes evaluation weights from the corresponding UCI parameters
  /// and setup king tables.

  void init() {

    Weights[PawnStructure]  = weight_option("Pawn Structure (Midgame)", "Pawn Structure (Endgame)", WeightsInternal[PawnStructure]);
    Weights[PassedPawns]    = weight_option("Passed Pawns (Midgame)", "Passed Pawns (Endgame)", WeightsInternal[PassedPawns]);
    Weights[Space]          = weight_option("Space", "Space", WeightsInternal[Space]);
    Weights[KingSafety]     = weight_option("King Safety", "King Safety", WeightsInternal[KingSafety]);

    const int MaxSlope = 8700;
    const int Peak = 1280000;
    int t = 0;

    for (int i = 0; i < 400; ++i)
    {
        t = std::min(Peak, std::min(i * i * 27, t + MaxSlope));
        KingDanger[i] = apply_weight(make_score(t / 1000, 0), Weights[KingSafety]);
    }
  }

} // namespace Eval
