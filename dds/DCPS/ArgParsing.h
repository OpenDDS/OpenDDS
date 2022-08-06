/**
 * @file ArgParsing.h
 *
 * Command line argument parsing loosely inspired by Python's argparse library.
 * Programs should create a ArgParser object and a Positional or Option object
 * for each kind of argument, then call the parse method of the ArgParser
 * object.
 *
 * See realistic example near the top of
 * tests/unit-tests/dds/DCPS/ArgParsing.cpp.
 *
 * Possible Improvements:
 *  - Make it possible to use this to process OpenDDS builtin options (shift mode)
 *  - More customizable usage and help
 *  - Subcommands and child parsers
 *  - Wrapper value handler for some multiple of the same kind of value.
 */

#ifndef OPENDDS_DCPS_ARG_PARSING_H
#define OPENDDS_DCPS_ARG_PARSING_H

#include "PoolAllocator.h"
#include "SafetyProfileStreams.h"
#include "dcps_export.h"

#include <dds/Versioned_Namespace.h>

#include <stdexcept>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <functional>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {
namespace ArgParsing {

bool starts_with(const String& str, const String& prefix);

/// Base class for all exceptions
class OpenDDS_Dcps_Export ArgParsingError : public std::exception {
public:
  explicit ArgParsingError(const String& what)
    : what_(what)
  {
  }

  virtual ~ArgParsingError() throw()
  {
  }

  virtual const char* what() const throw()
  {
    return what_.c_str();
  }

protected:
  String what_;
};

struct ArgParseState;

/// Thrown when parsed arguments are invalid
class OpenDDS_Dcps_Export ParseError : public ArgParsingError {
public:
  explicit ParseError(const String& what)
    : ArgParsingError(what)
  {
  }

  ParseError(const ArgParseState& state, const String& what);
};

/**
 * Thrown when arguments should cause the program to exit with a successful
 * exit status.
 */
class OpenDDS_Dcps_Export ExitSuccess : public ArgParsingError {
public:
  explicit ExitSuccess()
    : ArgParsingError("Not an error, program should exit early successfully")
  {
  }
};

typedef OPENDDS_VECTOR(String) StrVec;
typedef StrVec::iterator StrVecIt;

class ArgParser;

struct OpenDDS_Dcps_Export ArgParseState {
  ArgParseState(ArgParser& parser)
    : parser(parser)
  {
  }

  void use_argv(int argc, char* argv[]);

  const String& prog_name() const;

  ArgParser& parser;
  String argv0;
  StrVec args;
  // Name to use to reference the current argument in error messages
  String current_arg_ref;
};

class Argument;

const size_t size_t_max = (std::numeric_limits<size_t>::max)();

String OpenDDS_Dcps_Export count_required(size_t min_count, size_t max_count);

/*
 * Base class for handling found arguments.
 */
class OpenDDS_Dcps_Export Handler {
public:
  virtual ~Handler()
  {
  }

  virtual size_t min_args_required() const
  {
    return 1;
  }

  virtual size_t max_args_required() const
  {
    return size_t_max;
  }

  virtual String arg_count_required() const
  {
    return count_required(min_args_required(), max_args_required());
  }

  virtual String metavar() const
  {
    return String();
  }

  virtual String value_help() const
  {
    return "";
  }

  virtual StrVecIt handle(Argument& arg, ArgParseState& state, StrVecIt values) = 0;
};

class NullHandler : public Handler {
public:
  size_t min_args_required() const
  {
    return 0;
  }

  size_t max_args_required() const
  {
    return 0;
  }

  StrVecIt handle(Argument& /*arg*/, ArgParseState& /*state*/, StrVecIt values)
  {
    return values;
  }
};

class OpenDDS_Dcps_Export StringValue : public Handler {
public:
  typedef String ValueType;

  StringValue()
    : dest_(0)
  {
  }

  StringValue(String& dest)
    : dest_(&dest)
  {
  }

  size_t min_args_required() const
  {
    return 1;
  }

  size_t max_args_required() const
  {
    return 1;
  }

  StrVecIt handle(Argument& /*arg*/, ArgParseState& state, StrVecIt values)
  {
    if (values == state.args.end()) {
      return values;
    }
    if (dest_) {
      *dest_ = *values;
    }
    return state.args.erase(values);
  }

protected:
  String* const dest_;
};

template <typename Value>
class ChoiceValue : public StringValue {
public:
  typedef Value ValueType;

  struct Choice {
    String name;
    ValueType value;
    String help;
  };

  typedef OPENDDS_MAP(String, Choice) ChoicesMap;
  typedef OPENDDS_VECTOR(String) ChoicesVec;
  String str_choice;
  ChoicesMap choices_map;
  ChoicesVec choices_vec;
  ValueType& dest;

  ChoiceValue(ValueType& dest)
    : StringValue(str_choice)
    , dest(dest)
  {
  }

  void add_choice(const String& name, ValueType value, const String& help)
  {
    Choice choice;
    choice.name = name;
    choice.value = value;
    choice.help = help;
    if (!choices_map.insert(std::make_pair(name, choice)).second) {
      throw ArgParsingError("ChoiceValue has duplicate of " + name);
    }
    choices_vec.push_back(name);
  }

  const Choice* get_choice(const String& name) const
  {
    typename ChoicesMap::const_iterator it = choices_map.find(name);
    if (it == choices_map.end()) {
      return 0;
    }
    return &it->second;
  }

  String metavar() const
  {
    if (choices_map.empty()) {
      throw ArgParsingError("ChoiceValue has no choices!");
    }
    String rv;
    typename ChoicesVec::const_iterator it = choices_vec.begin();
    if (it != choices_vec.end()) {
      rv += get_choice(*it)->name;
      for (++it; it != choices_vec.end(); ++it) {
        rv += "|" + get_choice(*it)->name;
      }
    }
    return rv;
  }

  String choice_help(typename ChoicesVec::const_iterator it) const
  {
    const Choice* const choice = get_choice(*it);
    String rv = choice->name;
    if (choice->help.length()) {
      rv += ": " + choice->help;
    }
    return rv;
  }

  String value_help() const
  {
    if (choices_map.empty()) {
      throw ArgParsingError("ChoiceValue has no choices!");
    }
    typename ChoicesVec::const_iterator it = choices_vec.begin();
    String rv = "Valid choices are:\n" + choice_help(it);
    for (++it; it != choices_vec.end(); ++it) {
      rv += "\n" + choice_help(it);
    }
    return rv;
  }

  StrVecIt handle(Argument& /*arg*/, ArgParseState& state, StrVecIt values)
  {
    if (values == state.args.end()) {
      return values;
    }
    const String& name = *values;
    const Choice* const choice = get_choice(name);
    if (!choice) {
      throw ParseError(state, "was passed invalid value \"" + name  + "\".\n" + value_help());
    }
    if (dest_) {
      *dest_ = name;
    }
    if (dest_ != &str_choice) {
      str_choice = name;
    }
    dest = choice->value;
    return state.args.erase(values);
  }
};

class StringChoiceValue : public ChoiceValue<String> {
public:
  StringChoiceValue(String& dest)
    : ChoiceValue<String>(dest)
  {
  }

  void add_choice(const String& value, const String& help)
  {
    ChoiceValue<String>::add_choice(value, value, help);
  }
};

/// Base class for argument prototypes
class OpenDDS_Dcps_Export Argument {
public:
  bool show_in_help_;
  bool show_in_usage_;
  bool separate_usage_line_;

  Argument(ArgParser& arg_parser, const String& help, Handler* handler);

  virtual ~Argument()
  {
  }

  virtual String help() const;

  virtual void help(const String& value)
  {
    help_ = value;
  }

  virtual bool show_in_primary_usage_line() const
  {
    return show_in_usage_ && !separate_usage_line_;
  }

  virtual bool show_on_separate_usage_line() const
  {
    return show_in_usage_ && separate_usage_line_;
  }

  virtual size_t prototype_max_width(bool /*single_line*/) const = 0;

  virtual String prototype(size_t /*single_line_limit*/,
    bool /*force_single_line*/ = false) const = 0;

  virtual bool option() const
  {
    return false;
  }

  virtual bool positional() const
  {
    return false;
  }

  virtual bool optional() const = 0;

  size_t count() const
  {
    return count_;
  }

  void reset_count()
  {
    count_ = 0;
  }

  bool present() const
  {
    return count_ > 0;
  }

  size_t min_args_required() const
  {
    return handler_->min_args_required();
  }

  size_t max_args_required() const
  {
    return handler_->max_args_required();
  }

  virtual StrVecIt find(ArgParseState& state) = 0;

  virtual StrVecIt handle_find_result(ArgParseState& state, StrVecIt found) = 0;

  StrVecIt handle_found(ArgParseState& state, StrVecIt found_it);

  virtual bool process_args(ArgParseState& state);

protected:
  ArgParser& arg_parser_;
  String help_;
  Handler* const handler_;
  size_t count_;
};

typedef OPENDDS_VECTOR(Argument*) Arguments;
typedef Arguments::iterator ArgumentsIt;

/// Required and optional positional arguments that must be in a particular order
class OpenDDS_Dcps_Export Positional : public Argument {
public:
  Positional(ArgParser& arg_parser, const String& name, const String& help,
    Handler* handler, bool optional);
  Positional(ArgParser& arg_parser, const String& name, const String& help,
    Handler* handler);
  Positional(ArgParser& arg_parser, const String& name, const String& help, String& string_dest,
    bool optional);
  Positional(ArgParser& arg_parser, const String& name, const String& help, String& string_dest);

  const String& name() const
  {
    return name_;
  }

  void name(const String& new_name)
  {
    name_ = new_name;
  }

  size_t prototype_max_width(bool /*single_line*/) const
  {
    return prototype(0).length();
  }

  String prototype(size_t /*single_line_limit*/, bool /*force_single_line*/ = false) const
  {
    if (name_.empty()) {
      const String handler_metavar = handler_->metavar();
      if (handler_metavar.empty()) {
        throw ArgParsingError("Can't get Positional prototype, name is empty");
      }
      return handler_metavar;
    }
    return name_;
  }

  bool positional() const
  {
    return true;
  }

  bool optional() const
  {
    return optional_;
  }

  StrVecIt find(ArgParseState& state)
  {
    state.current_arg_ref = "positional argument " + name_;
    return state.args.begin();
  }

  StrVecIt handle_find_result(ArgParseState& /*state*/, StrVecIt found)
  {
    return found;
  }

protected:
  String name_;
  StringValue default_handler_;
  const bool optional_;
  String* const string_dest_;
};

template <typename HandlerType>
class PositionalDo : public Positional {
public:
  PositionalDo(ArgParser& arg_parser, const String& name, const String& help, bool optional)
    : Positional(arg_parser, name, help, &handler, optional)
    , handler()
  {
  }

  PositionalDo(ArgParser& arg_parser, const String& name, const String& help)
    : Positional(arg_parser, name, help, &handler)
    , handler()
  {
  }

  HandlerType handler;
};


template <typename HandlerType>
class PositionalAs : public Positional {
public:
  PositionalAs(ArgParser& arg_parser, const String& name, const String& help,
    typename HandlerType::ValueType& dest, bool optional)
    : Positional(arg_parser, name, help, dynamic_cast<Handler*>(&handler), optional)
    , handler(dest)
  {
  }

  PositionalAs(ArgParser& arg_parser, const String& name, const String& help,
    typename HandlerType::ValueType& dest)
    : Positional(arg_parser, name, help, dynamic_cast<Handler*>(&handler))
    , handler(dest)
  {
  }

  HandlerType handler;
};

enum OptionStyle {
  // Use ArgParser default
  OptionStyleDefault,
  // Tries to follow GNU style:
  // https://www.gnu.org/software/libc/manual/html_node/Argument-Syntax.html
  OptionStyleGnu,
  // The same as GNU, but using '-' instead of '--' to prefix long options,
  // which was apparently first used on Multics. Will also accept --.
  OptionStyleMultics,
};

class Option;
typedef OPENDDS_MAP(String, Option*) OptMap;

// Optional arguments that can appear in any order in between other arguments
class OpenDDS_Dcps_Export Option : public Argument {
public:
  /// Allow passing it multiple times, ignoring all except the last.
  bool allow_multiple_;

  /**
   * Separator for "attached values". By default it's '=', which allows
   * something like "--foo=bar" (option "foo" value "bar"). If cleared it
   * allows something like "-Da=b" (option "D" value "a=b").
   */
  String attached_value_separator_;

  Option(ArgParser& arg_parser, const String& name, const String& help,
    Handler* handler, OptionStyle style = OptionStyleDefault)
    : Argument(arg_parser, help, handler)
    , allow_multiple_(false)
    , attached_value_separator_("=")
    , style_(style)
    , present_dest_(0)
  {
    add_alias(name);
    show_in_usage_ = false;
  }

  Option(ArgParser& arg_parser, const String& name, const String& help, bool& present,
    OptionStyle style = OptionStyleDefault)
    : Argument(arg_parser, help, &default_handler_)
    , allow_multiple_(false)
    , attached_value_separator_("=")
    , style_(style)
    , present_dest_(&present)
  {
    add_alias(name);
    show_in_usage_ = false;
  }

  OptionStyle style() const;

  void style(OptionStyle new_style)
  {
    style_ = new_style;
  }

  void add_alias(const String& alias)
  {
    names_.push_back(alias);
  }

  String get_option_from_name(const String& name, OptionStyle style = OptionStyleDefault) const;

  String metavar() const
  {
    if (metavar_.empty()) {
      const String handler_metavar = handler_->metavar();
      if (handler_metavar.size()) {
        return handler_metavar;
      }
    }
    return metavar_;
  }

  void metavar(const String& new_metavar)
  {
    metavar_ = new_metavar;
  }

  size_t prototype_max_width(bool single_line) const;

  String prototype(size_t single_line_limit, bool force_single_line = false) const;

  bool option() const
  {
    return true;
  }

  bool optional() const
  {
    return true;
  }

  void add_all_aliases(OptMap& options, bool short_only = false);
  bool confirm(ArgParseState& state, const String& opt, StrVecIt found);

  StrVecIt find(ArgParseState& state);

  StrVecIt handle_find_result(ArgParseState& state, StrVecIt found);

protected:
  bool attached_value(const String& arg, const String& opt, String* value = 0);

  String get_prototype_from_name(const String& name) const;

  OptionStyle style_;
  NullHandler default_handler_;
  bool* present_dest_;
  StrVec names_;
  String metavar_;
};

template <typename HandlerType>
class OptionDo : public Option {
public:
  OptionDo(ArgParser& arg_parser, const String& name, const String& help)
    : Option(arg_parser, name, help, &handler)
    , handler()
  {
  }

  HandlerType handler;
};

template <typename HandlerType>
class OptionAs : public Option {
public:
  OptionAs(ArgParser& arg_parser, const String& name, const String& help,
    typename HandlerType::ValueType& dest, const String& metavar)
    : Option(arg_parser, name, help, &handler)
    , handler(dest)
  {
    this->metavar(metavar);
  }

  HandlerType handler;
};

#ifdef ACE_HAS_CPP11
template <typename HandlerType>
class OptionPass : public Option {
public:
  using ValueType = typename HandlerType::ValueType;
  using FuncType = std::function<void(const ValueType&)>;

  HandlerType handler;
  ValueType value;
  FuncType pass_to;

  OptionPass(ArgParser& arg_parser, const String& name, const String& help,
      FuncType pass_to, const String& metavar)
    : Option(arg_parser, name, help, &handler)
    , handler(value)
    , value()
    , pass_to(pass_to)
  {
    this->metavar(metavar);
  }

  StrVecIt handle_found(ArgParseState& state, StrVecIt found_it)
  {
    const bool found = found_it != state.args.end();
    const StrVecIt rv = Option::handle_found(state, found_it);
    if (found) {
      pass_to(value);
    }
    return rv;
  }
};
#endif

template <typename IntType>
class IntValue : public Handler {
protected:
  IntType* const dest_;

public:
  typedef IntType ValueType;
  typedef std::numeric_limits<IntType> Limits;

  IntType min_value;
  IntType max_value;

  IntValue(IntType& dest,
      IntType min_value = (Limits::min)(), IntType max_value = (Limits::max)())
    : dest_(&dest)
    , min_value(min_value)
    , max_value(max_value)
  {
  }

  size_t min_args_required() const
  {
    return 1;
  }

  size_t max_args_required() const
  {
    return 1;
  }

  StrVecIt handle(Argument& /*arg*/, ArgParseState& state, StrVecIt values)
  {
    if (values == state.args.end()) {
      return values;
    }
    const String& value = *values;
    if (value.empty()) {
      throw ParseError(state, "was passed an empty string, which is not a valid integer");
    }
    if (!Limits::is_signed && value[0] == '-') {
      throw ParseError(state, "was passed \"" + value +
        "\", which is not a positive integer");
    }
    IntType& dest = *dest_;
    if (!convertToInteger(value, dest)) {
      throw ParseError(state, "was passed \"" + value + "\", which is not a valid integer");
    }
    if (min_value > (Limits::min)() && dest < min_value) {
      throw ParseError(state, "was passed \"" + value + "\", which which is less than " +
        to_dds_string(min_value));
    }
    if (max_value < (Limits::max)() && dest > max_value) {
      throw ParseError(state, "was passed \"" + value + "\", which which is greater than " +
        to_dds_string(max_value));
    }
    return state.args.erase(values);
  }
};

/**
 * Try to wrap strings into paragraphs with a mostly limited width. It will
 * only wrap right_text, left_text is applied as is before the first right_text
 * line. If it overtakes right_indent then the left_text get its own lines
 * before right_text.
 *
 * Output takes the form of:
 * <-----------------------maxlen-------------------------->
 * <---------right_indent------>
 * <-left_indent->[left_text]   [right_text line 1         ]
 *                              [right_text line 2         ]
 *                              [...                       ]
 */
class OpenDDS_Dcps_Export WordWrapper {
public:
  WordWrapper(size_t left_indent, size_t right_indent, size_t maxlen)
    : left_indent_(left_indent)
    , right_indent_(right_indent)
    , maxlen_(maxlen)
  {
  }

  WordWrapper(size_t indent, size_t maxlen)
    : left_indent_(0)
    , right_indent_(indent)
    , maxlen_(maxlen)
  {
  }

  String wrap(const String& left_text, const String& right_text);

  String wrap(const String& text)
  {
    return wrap("", text);
  }

private:
  size_t left_indent_;
  size_t right_indent_;
  size_t maxlen_;
  String result_;
  String line_;
  String word_;
  StrVec left_lines_;
  StrVecIt left_line_;

  void add_word();
  size_t add_left_line();
  void add_line();
};

class OpenDDS_Dcps_Export HelpHandler : public NullHandler {
public:
  StrVecIt handle(Argument& arg, ArgParseState& state, StrVecIt values);
};

class OpenDDS_Dcps_Export VersionHandler : public NullHandler {
public:
  StrVecIt handle(Argument& arg, ArgParseState& state, StrVecIt values);
};

class OpenDDS_Dcps_Export ArgParser {
public:
  /// If not empty, this will override argv[0]
  String prog_name_;
  String desc_;
  /// If not empty, this will override the OpenDDS version
  String version_;
  std::ostream* out_stream_;
  std::ostream* error_stream_;
  bool use_positional_only_sentinal_;
  String positional_only_sentinal_;
  size_t max_line_length_;
  bool call_exit_;
  bool mention_opendds_options_;
  bool debug_;

  ArgParser(const String& desc = "");

  virtual ~ArgParser()
  {
  }

  void exit(int exit_status)
  {
    if (call_exit_) {
      std::exit(exit_status);
    }
  }

  void add_argument(Argument* arg);

  void start_optional_positionals()
  {
    optional_positionals_ = true;
  }

  bool optional_positionals() const
  {
    return optional_positionals_;
  }

  bool looks_like_option(const String& arg);

  bool is_positional_only_sentinal(const String& arg)
  {
    return use_positional_only_sentinal_ && arg == positional_only_sentinal_;
  }

  void parse_no_exit(int argc, char* argv[]);

  /**
   * Parse the supplied arguments from main. Returns true on success.
   *
   * By default parse will call std::exit if an invalid set of arguments
   * were passed or if the auto generated --help or --version options were
   * passed. If call_exit_ is false, then it will be able to return false on
   * invalid arguments.
   * @{
   */
  bool parse(int argc, char* argv[]);
#ifdef ACE_USES_WCHAR
  bool parse(int argc, ACE_TCHAR* argv[]);
#endif
  /// @}

  virtual void print_usage(ArgParseState& state, std::ostream& os);
  virtual void print_help(ArgParseState& state, std::ostream& os);
  virtual void print_version(std::ostream& os);

  static const OptionStyle default_default_option_style;

  OptionStyle default_option_style() const
  {
    return default_option_style_;
  }

  void default_option_style(OptionStyle new_style)
  {
    default_option_style_ = (new_style == OptionStyleDefault ?
      default_default_option_style : new_style);
  }

  void reset_arg_counts();

protected:
  virtual void parse_i(ArgParseState& state);

  OptionStyle default_option_style_;
  Arguments arguments_;
  bool optional_positionals_;
  OptionDo<HelpHandler> help_opt_;
  OptionDo<VersionHandler> version_opt_;
};

} // namespace ArgParsing
} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_ARG_PARSING_H
