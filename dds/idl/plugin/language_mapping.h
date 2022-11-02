#ifndef OPENDDS_IDL_LANGUAGE_MAPPING_H
#define OPENDDS_IDL_LANGUAGE_MAPPING_H

#include "opendds_idl_plugin_export.h"

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

protected:
  std::string to_macro(const char* fn) const;
  std::string to_header(const char* cpp_name) const;
  virtual void postprocess(const char* fn, std::ostringstream& content, int which) const;
};

#endif /* OPENDDS_IDL_LANGUAGE_MAPPING_H */
