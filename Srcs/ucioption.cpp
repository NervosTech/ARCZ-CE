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
#include <cassert>
#include <ostream>
#include <iostream>

#include "misc.h"
#include "search.h"
#include "thread.h"
#include "tt.h"
#include "uci.h"

#ifdef SYZYGY_TB
#include "syzygy/tbprobe.h"
#endif

#ifdef LOMONOSOV_TB
#include "lomonosov_probe.h"
#include "lmtb.h"
#endif

using std::string;

UCI::OptionsMap Options; // Global object

namespace UCI {

#ifdef LOMONOSOV_TB
#ifndef TB_DLL_EXPORT
	bool tb_stat = true;
#endif
#endif

/// 'On change' actions, triggered by an option's value change
void on_clear_hash(const Option&) { Search::clear(); }
void on_hash_size(const Option& o) { TT.resize(o); }
void on_logger(const Option& o) { start_logger(o); }
void on_threads(const Option&) { Threads.read_uci_options(); }

#ifdef SYZYGY_TB
void on_tb_path(const Option& o) { Tablebases::init(o); }
#endif
#ifdef LOMONOSOV_TB
void on_tb_used(const Option& o) {
	Tablebases::lomonosov_tb_use_opt = int(o);
}
void on_server_mode(const Option& o) {
	bool server_mode = bool(o);
	int result = lomonosov_change_server_mode(server_mode, Options["Lomonosov Server Console"]);
	sync_cout << "Lomonosov tables are" << (result == -1 ? " not" : "") << " loaded" << sync_endl;
}
void on_lomonosov_tb_path(const Option& o) {
	char path[MAX_PATH];
	strcpy(path, ((std::string)o).c_str());
	tb_add_table_path(((std::string)o).c_str());
	Tablebases::max_tb_pieces = tb_get_max_pieces_count_with_order();
	sync_cout << "Lomonosov_TB: " << "max pieces count is " << Tablebases::max_tb_pieces << sync_endl;
}
void on_tb_cache(const Option& o) {
	int cache = (int)o;
	tb_set_cache_size(cache);
}
void on_tb_order(const Option& o) {
	bool result = tb_set_table_order(((std::string)o).c_str());
	if (!result)
		sync_cout << "Lomonosov_TB: " << "Table order\"" << (std::string)o << "\" cannot set!" << sync_endl;
	Tablebases::max_tb_pieces = tb_get_max_pieces_count_with_order();
	sync_cout << "Lomonosov_TB: " << "Max pieces count is " << Tablebases::max_tb_pieces << sync_endl;
}
#ifndef TB_DLL_EXPORT
void on_tb_logging(const Option& o) {
	bool logging = int(o);
	tb_set_logging(logging);
}
void on_tb_stat(const Option& o) {
	tb_stat = int(o);
}
#endif
#endif

/// Our case insensitive less() function as required by UCI protocol
bool CaseInsensitiveLess::operator() (const string& s1, const string& s2) const {

  return std::lexicographical_compare(s1.begin(), s1.end(), s2.begin(), s2.end(),
         [](char c1, char c2) { return tolower(c1) < tolower(c2); });
}


/// init() initializes the UCI options to their hard-coded default values

void init(OptionsMap& o) {

  const int MaxHashMB = Is64Bit ? 1024 * 1024 : 2048;

  o["Write Debug Log"]       << Option(false, on_logger);
  o["Contempt"]              << Option(0, -100, 100);
  o["Threads"]               << Option(1, 1, 128, on_threads);
  o["Hash"]                  << Option(16, 1, MaxHashMB, on_hash_size);
  o["Clear Hash"]            << Option(on_clear_hash);
  o["Ponder"]                << Option(false);
  o["MultiPV"]               << Option(1, 1, 500);
  o["Skill Level"]           << Option(20, 0, 20);
  o["Move Overhead"]         << Option(25, 0, 5000);
  o["nodestime"]             << Option(0, 0, 10000);
  o["UCI_Chess960"]          << Option(false);
#ifdef SYZYGY_TB
  o["SyzygyPath"]            << Option("<empty>", on_tb_path);
  o["SyzygyProbeDepth"]      << Option(1, 1, 100);
  o["Syzygy50MoveRule"]      << Option(true);
  o["SyzygyProbeLimit"]      << Option(6, 0, 6);
#endif
#ifdef LOMONOSOV_TB
  o["Lomonosov Using"]       << Option(true, on_tb_used);
  o["Lomonosov Server Console"] << Option(false);
  o["Lomonosov Server Mode"] << Option(false, on_server_mode);
  o["Lomonosov Path"]        << Option("", on_lomonosov_tb_path);
  o["Lomonosov Cache"]       << Option(2048, 0, 32768, on_tb_cache);
  o["Lomonosov Order"]       << Option("PL;WL", on_tb_order);
  o["Lomonosov Depth Min"]   << Option(1, 1, 100);
  o["Lomonosov Depth Max"]   << Option(100, 1, 100);
#ifndef TB_DLL_EXPORT
  o["Lomonosov Logging"]     << Option(false, on_tb_logging);
  o["Lomonosov Stat"]        << Option(true, on_tb_stat);
#endif
#endif
}


/// operator<<() is used to print all the options default values in chronological
/// insertion order (the idx field) and in the format defined by the UCI protocol.

std::ostream& operator<<(std::ostream& os, const OptionsMap& om) {

  for (size_t idx = 0; idx < om.size(); ++idx)
      for (const auto& it : om)
          if (it.second.idx == idx)
          {
              const Option& o = it.second;
              os << "\noption name " << it.first << " type " << o.type;

              if (o.type != "button")
                  os << " default " << o.defaultValue;

              if (o.type == "spin")
                  os << " min " << o.min << " max " << o.max;

              break;
          }

  return os;
}


/// Option class constructors and conversion operators

Option::Option(const char* v, OnChange f) : type("string"), min(0), max(0), on_change(f)
{ defaultValue = currentValue = v; }

Option::Option(bool v, OnChange f) : type("check"), min(0), max(0), on_change(f)
{ defaultValue = currentValue = (v ? "true" : "false"); }

Option::Option(OnChange f) : type("button"), min(0), max(0), on_change(f)
{}

Option::Option(int v, int minv, int maxv, OnChange f) : type("spin"), min(minv), max(maxv), on_change(f)
{ defaultValue = currentValue = std::to_string(v); }

Option::operator int() const {
  assert(type == "check" || type == "spin");
  return (type == "spin" ? stoi(currentValue) : currentValue == "true");
}

Option::operator std::string() const {
  assert(type == "string");
  return currentValue;
}


/// operator<<() inits options and assigns idx in the correct printing order

void Option::operator<<(const Option& o) {

  static size_t insert_order = 0;

  *this = o;
  idx = insert_order++;
}


/// operator=() updates currentValue and triggers on_change() action. It's up to
/// the GUI to check for option's limits, but we could receive the new value from
/// the user by console window, so let's check the bounds anyway.

Option& Option::operator=(const string& v) {

  assert(!type.empty());

  if (   (type != "button" && v.empty())
      || (type == "check" && v != "true" && v != "false")
      || (type == "spin" && (stoi(v) < min || stoi(v) > max)))
      return *this;

  if (type != "button")
      currentValue = v;

  if (on_change)
      on_change(*this);

  return *this;
}

} // namespace UCI



/// Tuning Framework. Fully separated from SF code, appended here to avoid
/// adding a *.cpp file and to modify Makefile.

#include <iostream>
#include <sstream>

bool Tune::update_on_last;
const UCI::Option* LastOption = nullptr;
BoolConditions Conditions;
static std::map<std::string, int> TuneResults;

string Tune::next(string& names, bool pop) {

  string name;

  do {
      string token = names.substr(0, names.find(','));

      if (pop)
          names.erase(0, token.size() + 1);

      std::stringstream ws(token);
      name += (ws >> token, token); // Remove trailing whitespace

  } while (  std::count(name.begin(), name.end(), '(')
           - std::count(name.begin(), name.end(), ')'));

  return name;
}

static void on_tune(const UCI::Option& o) {

  if (!Tune::update_on_last || LastOption == &o)
      Tune::read_options();
}

static void make_option(const string& n, int v, const SetRange& r) {

  // Do not generate option when there is nothing to tune (ie. min = max)
  if (r(v).first == r(v).second)
      return;

  if (TuneResults.count(n))
      v = TuneResults[n];

  Options[n] << UCI::Option(v, r(v).first, r(v).second, on_tune);
  LastOption = &Options[n];

  // Print formatted parameters, ready to be copy-pasted in fishtest
  std::cout << n << ","
            << v << ","
            << r(v).first << "," << r(v).second << ","
            << (r(v).second - r(v).first) / 20.0 << ","
            << "0.0020"
            << std::endl;
}

template<> void Tune::Entry<int>::init_option() { make_option(name, value, range); }

template<> void Tune::Entry<int>::read_option() {
  if (Options.count(name))
      value = Options[name];
}

template<> void Tune::Entry<Value>::init_option() { make_option(name, value, range); }

template<> void Tune::Entry<Value>::read_option() {
  if (Options.count(name))
      value = Value(int(Options[name]));
}

template<> void Tune::Entry<Score>::init_option() {
  make_option("m" + name, mg_value(value), range);
  make_option("e" + name, eg_value(value), range);
}

template<> void Tune::Entry<Score>::read_option() {
  if (Options.count("m" + name))
      value = make_score(Options["m" + name], eg_value(value));

  if (Options.count("e" + name))
      value = make_score(mg_value(value), Options["e" + name]);
}

// Instead of a variable here we have a PostUpdate function: just call it
template<> void Tune::Entry<Tune::PostUpdate>::init_option() {}
template<> void Tune::Entry<Tune::PostUpdate>::read_option() { value(); }


// Set binary conditions according to a probability that depends
// on the corresponding parameter value.

void BoolConditions::set() {

  static PRNG rng(now());
  static bool startup = true; // To workaround fishtest bench

  for (size_t i = 0; i < binary.size(); i++)
      binary[i] = !startup && (values[i] + int(rng.rand<unsigned>() % variance) > threshold);

  startup = false;

  for (size_t i = 0; i < binary.size(); i++)
      sync_cout << binary[i] << sync_endl;
}


// Init options with tuning session results instead of default values. Useful to
// get correct bench signature after a tuning session or to test tuned values.
// Just copy fishtest tuning results in a result.txt file and extract the
// values with:
//
// cat results.txt | sed 's/^param: \([^,]*\), best: \([^,]*\).*/  TuneResults["\1"] = int(round(\2));/'
//
// Then paste the output below, as the function body

#include <cmath>

void Tune::read_results() {

  /* ...insert your values here... */
}