/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_IDL_FACE_LANGUAGE_MAPPING_H
#define OPENDDS_IDL_FACE_LANGUAGE_MAPPING_H

#include "language_mapping.h"
#include "be_builtin_global.h"

class FaceLanguageMapping: public LanguageMapping {
public:
  FaceLanguageMapping();
  virtual ~FaceLanguageMapping();

  virtual bool cxx11() const;
  virtual bool none() const;

  virtual std::string getMinimalHeaders() const;

  virtual std::string getInputCDRToString(bool wide) const;
  virtual std::string getBranchStringType(bool wide) const;
  virtual std::string getBranchStringPrefix() const;
  virtual std::string getBranchStringSuffix() const;
  virtual std::string getBoundStringSuffix() const;

  virtual bool needSequenceTypeSupportImplHeader() const;

  virtual void setTS(bool setting);
  virtual void produceTS() const;

  virtual GeneratorBase* getGeneratorHelper() const;

  enum FaceStreamType {
    STREAM_FACETS_H = BE_BuiltinGlobalData::STREAM_LAST_VALUE,
    STREAM_FACETS_CPP,
  };

  virtual Includes_t* additional_includes(int which);
  virtual void reset_includes();
  virtual void set_additional_names(const std::string& filebase);

private:
  Includes_t additional_h_;
  ACE_CString facets_header_name_, facets_impl_name_;
  bool emitTS_;

  virtual void postprocess_guard_begin(const std::string& macro, std::ostringstream& out, int which) const;
  virtual void postprocess_guard_end(const std::string& macro, std::ostringstream& out, int which) const;
};

#endif /* OPENDDS_IDL_FACE_LANGUAGE_MAPPING_H */
