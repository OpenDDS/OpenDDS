/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef TestUtils_Options_H
#define TestUtils_Options_H

#include "TestUtils_Export.h"

#include "tests/Utils/Options.h"

#include "dds/DdsDcpsPublicationC.h"
#include "dds/DdsDcpsSubscriptionC.h"
#include "dds/DdsDcpsTopicC.h"

#include <iostream>
#include <map>
#include <string>
#include <stdexcept>

namespace TestUtils {

class TestUtils_Export Arguments
{
public:
  enum Type { UNDEF, STRING, LONG, DOUBLE, BOOL };
  typedef std::pair<std::string, Type> ValueAndType;
  typedef std::map<std::string, ValueAndType>  Params;

  /// Create a class to define default parameters to receive on the command line
  /// @param define_all_parameters indicate if this class defines all arguments
  /// that can be passed on the command line, or only those that don't have to
  /// be present on the command line (and will thus cause an exception to be
  /// thrown if it is requested and is not present)
  Arguments(bool define_all_parameters = true);

  template<typename T>
  static ValueAndType value_and_type(T default_value)
  {
    const Type type = get_type(default_value);
    const std::string default_value_as_str = get_as_string(default_value);
    return ValueAndType(default_value_as_str, type);
  }

  static Type get_type(std::string ) { return STRING; }
  static Type get_type(bool ) { return BOOL; }
  static Type get_type(long ) { return LONG; }
  static Type get_type(double ) { return DOUBLE; }

  static std::string get_type_str(Type type);

  void add_long(const std::string& param, long val);
  void add_bool(const std::string& param, bool val);
  void add_double(const std::string& param, double val);
  void add_string(const std::string& param, const std::string& val);

  bool define_all_params() const;

  Params::const_iterator begin() const;
  Params::const_iterator end() const;
private:
  static std::string get_as_string(std::string str) { return str; }
  static std::string get_as_string(long val);
  static std::string get_as_string(double val);
  static std::string get_as_string(bool val);

  template<typename T>
  void add(const std::string& param, T default_value)
  {
    const ValueAndType vt = value_and_type(default_value);
    if (!params_.insert(std::make_pair(param, vt)).second) {
      std::cout << "ERROR: param=" << param
                << " defaulted more than once keeping original value";
    }
  }

  Params params_;
  const bool define_all_params_;
};

/// Class to parse non-DDS/TAO/ACE arguments and make them available
/// to an application
class TestUtils_Export Options
{
public:
  typedef Arguments::Params Params;

  Options();
  Options(int argc, ACE_TCHAR* argv[], const Arguments& args);
  Options(int argc, ACE_TCHAR* argv[]);

  /// parse and populate an Args with the provided command line (and Argument definition)
  void parse(int argc, ACE_TCHAR* argv[], const Arguments& args);
  void parse(int argc, ACE_TCHAR* argv[]);

  class TestUtils_Export Exception : public std::exception
  {
  public:
    Exception(const std::string& param, const std::string& reason);
    Exception(const std::string& reason);
    virtual ~Exception() throw ();
    virtual const char* what() const throw();
  private:
    std::string what_;
  };

  template<typename T>
  T get(const std::string& param)
  {
    Params::const_iterator data = params_.find(param);
    if (data == params_.end()) {
      if (define_all_params_)
        throw Exception(param, "param not defined as an accepted argument");

      T t;
      default_value(t);
      const Arguments::ValueAndType vt = Arguments::value_and_type(t);
      // now this becomes the default for now on
      data = params_.insert(std::make_pair(param, vt)).first;
    }

    // just using temp to distinguish which Type this is
    T temp;
    default_value(temp); // suppress uninit warning
    const Arguments::Type req_type = Arguments::get_type(temp);
    if (!define_all_params_ && data->second.second == Arguments::UNDEF) {
      params_.find(param)->second.second = req_type;
    }

    if (req_type != data->second.second) {
      std::string msg = "Param=";
      const std::string expected_type_str =
        Arguments::get_type_str(data->second.second);
      msg += param;
      msg += " previously indicated as type=";
      msg += expected_type_str;
      msg += " (in Arguments definition";
      if (define_all_params_) {
        msg += " or from previous call to get<";
        msg += expected_type_str;
        msg += ">";
      }
      msg += ") but requesting it as type=";
      msg += Arguments::get_type_str(req_type);
      msg += ", (value=";
      msg += data->second.first;
      msg += ").";
      throw Exception(param, msg);
    }

    return get_value<T>(data->second.first);
  }

private:
  template<typename T>
  T get_value(const std::string& val)
  {
    return val;
  }

  void default_value(std::string& val);
  void default_value(long& val);
  void default_value(double& val);
  void default_value(bool& val);

  Params params_;
  bool define_all_params_;
  bool args_parsed_;
};

template<>
inline long
Options::get_value<long>(const std::string& str_val)
{
  const long val = ACE_OS::atol(str_val.c_str());
  return val;
}

template<>
inline double
Options::get_value<double>(const std::string& str_val)
{
  const double val = ACE_OS::atof(str_val.c_str());
  return val;
}

template<>
inline bool
Options::get_value<bool>(const std::string& str_val)
{
  std::string lower = str_val;
  std::transform(lower.begin(), lower.end(), lower.begin(),::tolower);
  if (lower == "true")
    return true;
  else if (lower == "false")
    return false;
  else if (lower == "1")
    return true;

  return false;
}

} // End namespaces

#endif /* TestUtils_Options_H */
