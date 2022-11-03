/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_IDL_SP_LANGUAGE_MAPPING_H
#define OPENDDS_IDL_SP_LANGUAGE_MAPPING_H

#include "language_mapping.h"

class SPLanguageMapping: public LanguageMapping {
public:
  virtual ~SPLanguageMapping();

  virtual bool cxx11() const;

  virtual std::string getMinimalHeaders() const;

  virtual bool needSequenceTypeSupportImplHeader() const;
  virtual bool skipTAOSequences() const;

  virtual GeneratorBase* getGeneratorHelper() const;
};

#endif /* OPENDDS_IDL_SP_LANGUAGE_MAPPING_H */
