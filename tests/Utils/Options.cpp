/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Options.h"
#include "ace/Arg_Shifter.h"
#include <sstream>

namespace TestUtils {

Arguments::Arguments(bool define_all_parameters)
: define_all_params_(define_all_parameters)
{
}

std::string
Arguments::get_as_string(long val)
{
  char str_val[100];
  ACE_OS::itoa(val, str_val, 10);
  return str_val;
}

std::string
Arguments::get_as_string(double val)
{
  char str_val[100];
  ACE_OS::snprintf(str_val, sizeof str_val, "%f", val);
  return str_val;
}

std::string
Arguments::get_as_string(bool val)
{
  return val ? "true" : "false";
}

std::string
Arguments::get_type_str(Type type)
{
  switch (type) {
    case UNDEF:
      return "undefined";
    case STRING:
      return "string";
    case LONG:
      return "long";
    case DOUBLE:
      return "double";
    case BOOL:
      return "bool";
  };
  std::stringstream ss;
  ss << type;
  return "<undefined type=" + ss.str() + ">";
}

void
Arguments::add_long(const std::string& param, long val)
{
  add<long>(param, val);
}

void
Arguments::add_bool(const std::string& param, bool val)
{
  add<bool>(param, val);
}

void
Arguments::add_double(const std::string& param, double val)
{
  add<double>(param, val);
}

void
Arguments::add_string(const std::string& param, const std::string& val)
{
  add<std::string>(param, val);
}

bool
Arguments::define_all_params() const
{
  return define_all_params_;
}

Arguments::Params::const_iterator
Arguments::begin() const
{
  return params_.begin();
}

Arguments::Params::const_iterator
Arguments::end() const
{
  return params_.end();
}

Options::Options()
: define_all_params_(false)
, args_parsed_(false)
{
}

Options::Options(int argc, ACE_TCHAR* argv[], const Arguments& args)
: define_all_params_(false)
, args_parsed_(false)
{
  parse(argc, argv, args);
}

Options::Options(int argc, ACE_TCHAR* argv[])
: define_all_params_(false)
, args_parsed_(false)
{
  parse(argc, argv);
}

void
Options::parse(int argc, ACE_TCHAR* argv[], const Arguments& args)
{
  std::cout << "Options::parse with Arguments\n";
  define_all_params_ = args.define_all_params();
  for (Params::const_iterator arg = args.begin();
       arg != args.end();
       ++arg) {
    params_.insert(std::make_pair(arg->first, arg->second));
  }
//  std::copy(args.begin(), args.end(), params_.begin());
  parse(argc, argv);
}

void
Options::parse(int argc, ACE_TCHAR* argv[])
{
  if (args_parsed_) {
    throw Exception("Args cannot parse more than one set of arguments");
  }

  std::cerr << "Options::parse\n";
  for (int arg = 0; arg < argc; ++arg) {
    std::string param = ACE_TEXT_ALWAYS_CHAR(argv[arg]);
    if(!param.empty() && param[0] == '-') {
      param = param.substr(1); // strip off "-"
      std::string value;
      if (arg + 1 < argc) {
        // look ahead to see if flag or value
        value = ACE_TEXT_ALWAYS_CHAR(argv[arg + 1]);
        if(!value.empty() && value[0] == '-') {
          value = "true";
        } else {
           // already processed this arg
           ++arg;
        }
      } else {
        // param flag without a value, assume boolean true
        value = "true";
      }
      Arguments::ValueAndType vt(value, Arguments::UNDEF);
      std::pair<Params::iterator, bool> insert =
        params_.insert(std::make_pair(param, vt));
      if (insert.second) {
        if (define_all_params_)
          throw Exception(param, "Undeclared parameter");
      }
      else {
        // replace the default value with the real value
        insert.first->second.first = value;
      }
    } else {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Options::parse skipping value=%C not paired with flag\n"), param.c_str()));
    }
  }

  ACE_DEBUG((LM_DEBUG, "(%P|%t) parsed:\n"));
  for (Params::const_iterator param = params_.begin();
       param != params_.end();
       ++param) {
    ACE_DEBUG((LM_DEBUG,
               "(%P|%t) %C=%C (type=%C)\n",
               param->first.c_str(),
               param->second.first.c_str(),
               Arguments::get_type_str(param->second.second).c_str()));
  }
  args_parsed_ = true;
}

void
Options::default_value(std::string& )
{
}

void
Options::default_value(long& val)
{
  val = 0;
}

void
Options::default_value(double& val)
{
  val = 0.0;
}

void
Options::default_value(bool& val)
{
  val = false;
}

Options::Exception::Exception(const std::string& param, const std::string& reason)
{
  what_ = "ERROR: Options failed because ";
  what_ += reason;
  what_ += "for param=";
  what_ += param;
}

Options::Exception::Exception(const std::string& reason)
{
  what_ = "ERROR: Options failed because ";
  what_ += reason;
}

Options::Exception::~Exception() throw ()
{
}

const char*
Options::Exception::what() const throw()
{
  return what_.c_str();
}

} // End namespaces
