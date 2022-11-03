/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_IDL_LANGUAGE_MAPPING_H
#define OPENDDS_IDL_LANGUAGE_MAPPING_H

#include "opendds_idl_plugin_export.h"

#include <set>
#include <string>
#include <sstream>

class GeneratorBase;

class opendds_idl_plugin_Export LanguageMapping {
public:
  virtual ~LanguageMapping();

  virtual bool cxx11() const;
  virtual bool none() const;

  virtual std::string getMinimalHeaders() const;

  virtual std::string getInputCDRToString(bool wide) const;
  virtual std::string getBranchStringType(bool wide) const;
  virtual std::string getBranchStringPrefix() const;
  virtual std::string getBranchStringSuffix() const;
  virtual std::string getBoundStringSuffix() const;

  virtual bool skipTAOSequences() const;
  virtual bool needSequenceTypeSupportImplHeader() const;

  virtual void setTS(bool setting);
  virtual void produce() const;
  virtual void produceTS() const;

  virtual GeneratorBase* getGeneratorHelper() const;

  typedef std::set<std::pair<std::string, std::string> > Includes_t;
  virtual Includes_t* additional_includes(int which);
  virtual void reset_includes();
  virtual void set_additional_names(const std::string& filebase);

  virtual void usage() const;

protected:
  std::string to_macro(const char* fn) const;
  std::string to_header(const char* cpp_name) const;
  void emit_tao_header(std::ostringstream& out) const;

  virtual void postprocess(const char* fn, const std::ostringstream& content, int which) const;
  virtual void postprocess_guard_begin(const std::string& macro, std::ostringstream& content, int which) const;
  virtual void postprocess_guard_end(const std::string& macro, std::ostringstream& content, int which) const;
};

#endif /* OPENDDS_IDL_LANGUAGE_MAPPING_H */
