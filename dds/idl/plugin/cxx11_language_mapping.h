/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_IDL_CXX11_LANGUAGE_MAPPING_H
#define OPENDDS_IDL_CXX11_LANGUAGE_MAPPING_H

#include "language_mapping.h"

class Cxx11LanguageMapping: public LanguageMapping {
public:
  virtual ~Cxx11LanguageMapping();

  virtual bool cxx11() const;
  virtual bool none() const;

  virtual bool needSequenceTypeSupportImplHeader() const;

  virtual GeneratorBase* getGeneratorHelper() const;
};

#endif /* OPENDDS_IDL_CXX11_LANGUAGE_MAPPING_H */
