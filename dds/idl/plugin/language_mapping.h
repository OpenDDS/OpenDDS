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

  typedef void (*postprocess)(const char* fn,
                              std::ostringstream& content, int which);

  virtual void setTS(bool setting);
  virtual void produceTS(postprocess func) const;

  virtual GeneratorBase* getGeneratorHelper() const;
};

#endif /* OPENDDS_IDL_LANGUAGE_MAPPING_H */
