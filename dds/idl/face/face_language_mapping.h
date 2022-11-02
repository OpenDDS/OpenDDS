#ifndef OPENDDS_IDL_FACE_LANGUAGE_MAPPING_H
#define OPENDDS_IDL_FACE_LANGUAGE_MAPPING_H

#include "language_mapping.h"

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
  virtual void produceTS(postprocess func) const;

  virtual GeneratorBase* getGeneratorHelper() const;

private:
  bool emitTS_;
};

#endif /* OPENDDS_IDL_FACE_LANGUAGE_MAPPING_H */
