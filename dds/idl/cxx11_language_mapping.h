#ifndef OPENDDS_IDL_CXX11_LANGUAGE_MAPPING_H
#define OPENDDS_IDL_CXX11_LANGUAGE_MAPPING_H

#include "language_mapping.h"

class Cxx11LanguageMapping: public LanguageMapping {
public:
  virtual ~Cxx11LanguageMapping();

  virtual bool cxx11() const;
  virtual bool none() const;

  virtual std::string getMinimalHeaders() const;

  virtual std::string getInputCDRToString(bool wide) const;
  virtual std::string getBranchStringType(bool wide) const;
  virtual std::string getBranchStringPrefix() const;

  virtual bool needSequenceTypeSupportImplHeader() const;

  virtual GeneratorBase* getGeneratorHelper() const;
};

#endif /* OPENDDS_IDL_CXX11_LANGUAGE_MAPPING_H */
