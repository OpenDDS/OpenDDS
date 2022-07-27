#include "ArgParsing.h"

#include <dds/Version.h>

#include <ace/Log_Msg.h>
#ifdef ACE_USES_WCHAR
#  include <ace/Argv_Type_Converter.h>
#endif

#include <cstring>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {
namespace ArgParsing {

bool starts_with(const String& str, const String& prefix)
{
  if (prefix.length() > str.length()) {
    return false;
  }
  return !std::strncmp(str.c_str(), prefix.c_str(), prefix.length());
}

ParseError::ParseError(const ArgParseState& state, const String& what)
  : ArgParsingError(state.current_arg_ref + " " + what)
{
}

void ArgParseState::use_argv(int argc, char* argv[])
{
  if (argc == 0 || !argv[0]) {
    throw ArgParsingError("argv should not be empty");
  }
  if (!argv[0][0]) {
    throw ArgParsingError("argv[0] should not be empty");
  }
  argv0 = argv[0];
  for (int i = 1; i < argc; ++i) {
    args.push_back(argv[i]);
  }
}

const String& ArgParseState::prog_name() const
{
  if (parser.prog_name_.length()) {
    return parser.prog_name_;
  }
  if (argv0.empty()) {
    throw ArgParsingError("cannot get program name, argv0 and parser.prog_name are empty");
  }
  return argv0;
}

String count_required(size_t min_count, size_t max_count)
{
  if (min_count == max_count) {
    return min_count ? "exactly " + to_dds_string(min_count) : "no";
  } else if (max_count == size_t_max) {
    return min_count ? "at least " + to_dds_string(min_count) : "any number of";
  } else if (min_count == 0 && max_count < size_t_max) {
    return "at most " + to_dds_string(max_count);
  } else {
    return "between " + to_dds_string(min_count) + " and " + to_dds_string(max_count);
  }
}

void StringChoiceValue::handle(Argument& /*arg*/, ArgParseState& state, StrVecIt values)
{
  if (values == state.args.end()) {
    return;
  }
  String value = *values;
  if (!choices.count(value)) {
    throw ParseError(state, "was passed invalid value \"" + value + "\"");
  }
  if (dest_) {
    *dest_ = value;
  }
  state.args.erase(values);
}

Argument::Argument(ArgParser& arg_parser, const String& help,
  Handler* handler)
  : show_in_help_(true)
  , show_in_usage_(true)
  , separate_usage_line_(false)
  , arg_parser_(arg_parser)
  , help_(help)
  , handler_(handler)
  , present_(false)
{
  arg_parser.add_argument(this);
}

bool Argument::process_args(ArgParseState& state)
{
  if (state.parser.debug_) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) Argument::process_args: find \"%C\", arguments left:\n",
      prototype(0, true).c_str()));
    for (StrVecIt it = state.args.begin(); it != state.args.end(); ++it) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) Argument::process_args: \"%C\"\n", it->c_str()));
    }
  }

  // Find the argument
  state.current_arg_ref.clear();
  StrVecIt found_it = find(state);
  const bool found = found_it != state.args.end();
  if (found) {
    present_ = true;
  }
  if (!found) {
    state.current_arg_ref.clear();
  } else if (state.current_arg_ref.empty()) {
    throw ArgParsingError("current_arg_ref is empty!");
  } else if (state.parser.debug_) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) Argument::process_args: "
      "found \"%C\"\n", prototype(0, true).c_str()));
  }

  // Take the start of the argument and transform into the values
  // handle_find_result should be called even if the argument isn't found so
  // the argument can handle that case if it needs to.
  StrVecIt values = handle_find_result(state, found_it);

  // Handle the presence and the values passed in with the argument
  if (found) {
    const size_t left = state.args.end() - values;
    if (handler_->min_args_required() > left) {
      throw ParseError(state, "requires " + handler_->arg_count_required() +
        " argument(s) to be passed after it, but there are only " + to_dds_string(left));
    }
    handler_->handle(*this, state, values);
  }

  return found;
}

Positional::Positional(ArgParser& arg_parser, const String& name, const String& help,
  Handler* handler, bool optional)
  : Argument(arg_parser, help, handler)
  , name_(name)
  , optional_(optional)
  , string_dest_(0)
{
}

Positional::Positional(ArgParser& arg_parser, const String& name, const String& help,
  Handler* handler)
  : Argument(arg_parser, help, handler)
  , name_(name)
  , optional_(arg_parser.optional_positionals())
  , string_dest_(0)
{
}

Positional::Positional(ArgParser& arg_parser, const String& name, const String& help,
  String& string_dest, bool optional)
  : Argument(arg_parser, help, &default_handler_)
  , name_(name)
  , default_handler_(string_dest)
  , optional_(optional)
  , string_dest_(&string_dest)
{
}

Positional::Positional(ArgParser& arg_parser, const String& name, const String& help,
  String& string_dest)
  : Argument(arg_parser, help, &default_handler_)
  , name_(name)
  , default_handler_(string_dest)
  , optional_(arg_parser.optional_positionals())
  , string_dest_(&string_dest)
{
}

OptionStyle Option::style() const
{
  return style_ == OptionStyleDefault ? arg_parser_.default_option_style() : style_;
}

String Option::get_option_from_name(const String& name) const
{
  return ((name.length() > 1 && style() == OptionStyleGnu) ? "--" : "-") + name;
}

namespace {
  const String single_line_join(" | ");
}

size_t Option::prototype_max_width(bool single_line) const
{
  size_t width = 0;
  StrVec::const_iterator it = names_.begin();
  width = get_prototype_from_name(*it).length();
  for (++it; it != names_.end(); ++it) {
    if (single_line) {
      width += single_line_join.length() + get_prototype_from_name(*it).length();
    } else {
      width = std::max(width, get_prototype_from_name(*it).length());
    }
  }
  return width;
}

String Option::prototype(size_t single_line_limit, bool force_single_line) const
{
  // Figure out if we can do single line join
  const bool do_single_line_join = force_single_line ||
    (single_line_limit && prototype_max_width(true) <= single_line_limit);

  // Now actually make the string
  StrVec::const_iterator it = names_.begin();
  it = names_.begin();
  String rv = get_prototype_from_name(*it);
  for (++it; it != names_.end(); ++it) {
    rv += (do_single_line_join ? single_line_join : "\n") + get_prototype_from_name(*it);
  }
  if (!do_single_line_join) {
    rv += '\n';
  }

  return rv;
}

bool Option::attached_value(const String& arg, const String& opt, String* value)
{
  const String optsep = opt + attached_value_seperator_;
  const bool attached = starts_with(arg, optsep);

  if (attached && value) {
    *value = arg.substr(optsep.length());
  }

  return attached;
}

String Option::get_prototype_from_name(const String& name) const
{
  const String mv = metavar();
  return get_option_from_name(name) + (mv.length() ? " " + mv : "");
}

StrVecIt Option::find(ArgParseState& state)
{
  for (StrVecIt it = state.args.begin(); it != state.args.end(); ++it) {
    if (state.parser.is_positional_only_sentinal(*it)) {
      // Remaining arguments should be interpreted as positional arguments
      break;
    }

    for (StrVecIt name_it = names_.begin(); name_it != names_.end(); ++name_it) {
      const String opt = get_option_from_name(*name_it);
      if (*it == opt || attached_value(*it, opt)) {
        state.current_arg_ref = "option " + opt;
        return it;
      }
    }
  }

  return state.args.end();
}

StrVecIt Option::handle_find_result(ArgParseState& state, StrVecIt found)
{
  if (!present_ || found == state.args.end()) {
    return state.args.end();
  }
  if (present_dest_) {
    *present_dest_ = present_;
  }

  // Get attached value if it's there
  for (StrVecIt name_it = names_.begin(); name_it != names_.end(); ++name_it) {
    const String opt = get_option_from_name(*name_it);
    String value;
    if (attached_value(*found, opt, &value)) {
      if (handler_->max_args_required() == 0) {
        throw ParseError(state, "has the value \"" + value +
          "\" attached, but isn't supposed to be passed anything");
      } else if (handler_->min_args_required() > 1) {
        throw ParseError(state, "has the value \"" + value + "\" attached, but requires " +
          handler_->arg_count_required() + " arguments, so this form (" +
          *found + ") is invalid because it requires multiple arguments");
      }
      // If a value is attached to the option flag then the value replaces the
      // whole thing.
      *found = value;
      return found;
    }
  }

  // If the first value is separate, we need to erase the option flag to get to
  // the start of the possible values.
  return state.args.erase(found);
}

bool Option::process_args(ArgParseState& state)
{
  const bool found = Argument::process_args(state);
  if (found && allow_multiple_) {
    while (Argument::process_args(state)) {
    }
  } else {
    if (Argument::process_args(state)) {
      throw ParseError(state, "was passed multiple times");
    }
  }
  return found;
}

String WordWrapper::wrap(const String& left_text, const String& right_text)
{
  result_.clear();
  line_.clear();
  word_.clear();
  left_lines_.clear();

  size_t max_line = 0;
  size_t here = 0;
  size_t there = 0;
  while (here < left_text.length()) {
    there = left_text.find_first_of('\n', here);
    size_t next;
    if (there == String::npos) {
      there = left_text.length();
      next = there;
    } else {
      next = there + 1;
    }
    max_line = std::max(max_line, left_indent_ + there - here);
    left_lines_.push_back(left_text.substr(here, there - here));
    here = next;
  }
  left_line_ = left_lines_.begin();
  if (max_line >= right_indent_) {
    for (; left_line_ != left_lines_.end();) {
      add_left_line();
      result_ += '\n';
    }
  }

  for (const char* c = &right_text[0]; *c; ++c) {
    if (std::isspace(*c)) {
      add_word();
      if (*c == '\n') {
        add_line();
      }
    } else {
      word_ += *c;
    }
  }
  add_word();
  add_line();

  for (; left_line_ != left_lines_.end();) {
    add_left_line();
    result_ += '\n';
  }

  return result_;
}

void WordWrapper::add_word()
{
  if (!word_.length()) {
    return;
  }
  if (right_indent_ + line_.length() + 1 + word_.length() > maxlen_) {
    add_line();
    line_ = word_;
  } else {
    if (line_.length()) {
      line_ += ' ';
    }
    line_ += word_;
  }
  word_.clear();
}

size_t WordWrapper::add_left_line()
{
  String precede;
  if (left_line_ != left_lines_.end()) {
    if (left_line_->length()) {
      precede = String(left_indent_, ' ') + *left_line_;
    }
    result_ += precede;
    ++left_line_;
  }
  return precede.length();
}

void WordWrapper::add_line()
{
  if (line_.length()) {
    result_ += String(right_indent_ - add_left_line(), ' ') + line_;
  }
  result_ += '\n';
  line_.clear();
}

void HelpHandler::handle(Argument& /*arg*/, ArgParseState& state, StrVecIt /*values*/)
{
  state.parser.print_help(state, *state.parser.out_stream_);
  throw ExitSuccess();
}

void VersionHandler::handle(Argument& /*arg*/, ArgParseState& state, StrVecIt /*values*/)
{
  state.parser.print_version(*state.parser.out_stream_);
  throw ExitSuccess();
}

const OptionStyle ArgParser::default_default_option_style = OptionStyleGnu;

ArgParser::ArgParser(const String& desc)
  : desc_(desc)
  , version_(OPENDDS_VERSION)
  , out_stream_(&std::cout)
  , error_stream_(&std::cerr)
  , use_positional_only_sentinal_(true)
  , positional_only_sentinal_("--")
  , max_line_length_(80)
  , call_exit_(true)
  , mention_opendds_options_(true)
  , debug_(false)
  , default_option_style_(ArgParser::default_default_option_style)
  , optional_positionals_(false)
  , help_opt_(*this, "help", "Print this message.")
  , version_opt_(*this, "version", "Print the program version.")
{
  help_opt_.add_alias("h");
  help_opt_.show_in_usage_ = true;
  help_opt_.separate_usage_line_ = true;
  version_opt_.add_alias("v");
  version_opt_.show_in_usage_ = true;
  version_opt_.separate_usage_line_ = true;
}

void ArgParser::add_argument(Argument* arg)
{
  if (arg->positional()) {
    if (!arg->optional()) {
      if (optional_positionals_) {
        throw ArgParsingError("Can't add required positional arguments after optional ones");
      }
    } else {
      optional_positionals_ = true;
    }
  }
  arguments_.push_back(arg);
}

bool ArgParser::looks_like_option(const String& arg)
{
  if (starts_with(arg, "-")) {
    // Ignore it if it's a negative number
    long int_value;
    if (convertToInteger(arg, int_value) && int_value < 0) {
      return false;
    }
    return true;
  }
  return false;
}

void ArgParser::parse_i(ArgParseState& state)
{
  // Parse options
  for (ArgumentsIt it = arguments_.begin(); it != arguments_.end(); ++it) {
    if ((*it)->option()) {
      (*it)->process_args(state);
    }
  }

  // Check what's left over for invalid options
  for (StrVecIt it = state.args.begin(); it != state.args.end(); ++it) {
    if (state.parser.is_positional_only_sentinal(*it)) {
      // Remaining arguments should be interpreted as positional arguments
      state.args.erase(it);
      break;
    }
    if (looks_like_option(*it)) {
      throw ParseError("option " + *it + " is invalid");
    }
  }

  // TODO: shift mode would modify argc and argv and exit here

  // Check positional argument count
  size_t min_positional_arg_count = 0;
  size_t max_positional_arg_count = 0;
  for (ArgumentsIt it = arguments_.begin(); it != arguments_.end(); ++it) {
    if ((*it)->positional()) {
      if (!(*it)->optional()) {
        min_positional_arg_count += (*it)->min_args_required();
      }
      max_positional_arg_count += (*it)->max_args_required();
    }
  }
  if (state.args.size() < min_positional_arg_count ||
      state.args.size() > max_positional_arg_count) {
    throw ParseError("expected " +
      count_required(min_positional_arg_count, max_positional_arg_count) +
      " positional argument(s), received " + to_dds_string(state.args.size()));
  }

  // Parse positional arguments
  for (ArgumentsIt it = arguments_.begin(); it != arguments_.end(); ++it) {
    if ((*it)->positional()) {
      (*it)->process_args(state);
    }
  }

  // TODO: If configured return leftover ages here
}

void ArgParser::parse_no_exit(int argc, char* argv[])
{
  ArgParseState state(*this);
  state.use_argv(argc, argv);
  parse_i(state);
}

bool ArgParser::parse(int argc, char* argv[])
{
  WordWrapper error_ww(0, max_line_length_);
  ArgParseState state(*this);
  try {
    state.use_argv(argc, argv);
    parse_i(state);
  } catch (const ExitSuccess&) {
    exit(0);
  } catch (const ParseError& e) {
    *error_stream_ << error_ww.wrap(String("ERROR: ") + e.what()) << std::endl;
    print_usage(state, *error_stream_);
    *error_stream_ << "See --help for more information" << std::endl;
    exit(1);
    return false;
  } catch (const ArgParsingError& e) {
    *error_stream_ << error_ww.wrap(
      String("ERROR: this is internal to ArgParsing: ") + e.what()) << std::endl;
    exit(1);
    return false;
  }
  return true;
}

#ifdef ACE_USES_WCHAR
bool ArgParser::parse(int argc, ACE_TCHAR* argv[])
{
  ACE_Argv_Type_Converter atc(argc_copy, argv);
  return parse(atc.get_argc(), atc.get_ASCII_argv());
}
#endif

namespace {
  const String opendds_options_blurb =
    "All OpenDDS command line options listed in section 7.2 of the OpenDDS "
    "Developer's Guide are also available.";
  const size_t help_min_indent = 2;
  const size_t help_max_indent = 30;

  void print_argument_usage(std::ostream& os, Argument* arg, bool show_braces = true)
  {
    os << ' ';
    const bool braces = show_braces && arg->optional();
    if (braces) {
      os << '[';
    }
    os << arg->prototype(help_max_indent, true);
    if (braces) {
      os << ']';
    }
  }

  void print_argument_help(std::ostream& os, Argument* arg, WordWrapper& ww)
  {
    os << ww.wrap(arg->prototype(help_max_indent), arg->help());
  }
}

void ArgParser::print_usage(ArgParseState& state, std::ostream& os)
{
  const std::string indent(help_min_indent, ' ');
  os << "Usage:\n" << indent << state.prog_name();
  for (ArgumentsIt it = arguments_.begin(); it != arguments_.end(); ++it) {
    if ((*it)->option() && !(*it)->show_in_usage_) {
      os << " [OPTION...]";
      break;
    }
  }
  for (ArgumentsIt it = arguments_.begin(); it != arguments_.end(); ++it) {
    if ((*it)->show_in_primary_usage_line()) {
      print_argument_usage(os, *it);
    }
  }
  os << std::endl;

  for (ArgumentsIt it = arguments_.begin(); it != arguments_.end(); ++it) {
    if ((*it)->show_on_separate_usage_line()) {
      os << indent << state.prog_name();
      print_argument_usage(os, *it, false);
      os << std::endl;
    }
  }
}

void ArgParser::print_help(ArgParseState& state, std::ostream& os)
{
  print_usage(state, os);

  WordWrapper normal_ww(help_min_indent, max_line_length_);
  if (desc_.length()) {
    os << "\nDescription:\n" << normal_ww.wrap(desc_);
  }

  // Figure the out indent of the text on the left
  size_t help_indent = 0;
  for (ArgumentsIt it = arguments_.begin(); it != arguments_.end(); ++it) {
    if ((*it)->show_in_help_) {
      const size_t single_line_width = (*it)->prototype_max_width(true);
      const size_t multi_line_width = (*it)->prototype_max_width(false);
      if (single_line_width <= help_max_indent) {
        help_indent = std::max(help_indent, single_line_width);
      } else if (multi_line_width <= help_max_indent) {
        help_indent = std::max(help_indent, multi_line_width);
      } else {
      }
      /* help_indent = std::max(single_line_len <= help_max_indent); */
    }
  }
  const size_t name_indent = help_min_indent;
  const size_t name_help_sep = 2;
  help_indent += name_indent + name_help_sep;
  WordWrapper arg_ww(name_indent, help_indent, max_line_length_);

  // Print positional arguments
  bool printed_pos = false;
  for (ArgumentsIt it = arguments_.begin(); it != arguments_.end(); ++it) {
    if ((*it)->positional() && (*it)->show_in_help_) {
      if (!printed_pos) {
        os << "\nPositional arguments:\n";
        printed_pos = true;
      }
      print_argument_help(os, *it, arg_ww);
    }
  }

  // Print option arguments
  bool printed_opt = false;
  for (ArgumentsIt it = arguments_.begin(); it != arguments_.end(); ++it) {
    if ((*it)->option() && (*it)->show_in_help_) {
      if (!printed_opt) {
        os << "\nOptions:\n";
        if (mention_opendds_options_) {
          os << normal_ww.wrap(opendds_options_blurb);
        }
        printed_opt = true;
      }
      print_argument_help(os, *it, arg_ww);
    }
  }
}

void ArgParser::print_version(std::ostream& os)
{
  os << "Version " << version_ << std::endl;
}

} // namespace ArgParsing
} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
