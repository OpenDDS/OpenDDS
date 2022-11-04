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

/// This is only a partial abstraction of the language mapping.
/// The language mapping code is spread out amongst many files.
/// This is a replacement for the LanguageMapping that used to
/// be in the BE_GlobalData structure.
class opendds_idl_plugin_Export LanguageMapping {
public:
  virtual ~LanguageMapping();

  /// Is this language mapping considered C++11?
  virtual bool cxx11() const;

  /// Is this the default language mapping?
  virtual bool none() const;

  /// Return code to include minimal headers in the generated
  /// header file.
  virtual std::string getMinimalHeaders() const;

  /// Return the serializer name for a bounded string in a union case body.
  virtual std::string getInputCDRToString(bool wide) const;

  /// Return the string type for a bounded string in a union case body.
  virtual std::string getBoundStringType(bool wide) const;

  /// Return the string namespace for an unbounded string in a union case body.
  virtual std::string getBranchStringPrefix() const;

  /// Return the string suffix for a bounded string in a union case body.
  virtual std::string getBoundStringSuffix() const;

  /// Return the string suffix for a bounded string with use with a copy
  /// operation.
  virtual std::string getBoundStringCopySuffix() const;

  /// Return true if the language mapping must skip the include of tao/*SeqC.h
  virtual bool skipTAOSequences() const;

  /// Return true if the language mapping needs to include the
  /// TypeSupportImpl.h in the generated .cpp file.
  virtual bool needSequenceTypeSupportImplHeader() const;

  /// Indicate that the language mapping should generate a TS API for DCPS
  /// data types, if applicable.
  virtual void setTS(bool setting);

  /// Perform the general code generation for this language mapping.
  virtual void produce() const;

  /// Provides the language mapping the opportunity to generate the TS API.
  virtual void produceTS() const;

  /// Provide a code generation helper with a range of operations.
  /// See language_generator_helper.h
  virtual GeneratorBase* getGeneratorHelper() const;

  typedef std::set<std::pair<std::string, std::string> > Includes_t;

  /// Provide a pointer to include file sets based on a unique selector.
  virtual Includes_t* additional_includes(int which);

  /// Reset all include file sets.
  virtual void reset_includes();

  /// Gives the language mapping the opportunity to set internal file names
  /// given the basename of the input file.
  virtual void set_additional_names(const std::string& filebase);

protected:
  /// Convert a file name to a macro (with randomization) to be used as an
  /// include guard.
  std::string to_macro(const char* fn) const;

  /// Return the header name based on the .cpp file name.
  std::string to_header(const char* cpp_name) const;

  /// Emit the inclusion of the tao_idl generated header (based on the name of
  /// the TypeSupportImpl.h file name.
  void emit_tao_header(std::ostringstream& out) const;

  /// Given the generated code "content", perform some final processing and
  /// write it to the designated file.
  virtual void postprocess(const char* fn, const std::ostringstream& content, int which) const;

  /// Handle initial guard generation for streams that are not included in the
  /// BE_BuiltinGlobalData structure.  See stream_enum_t for more information.
  virtual void postprocess_guard_begin(const std::string& macro, std::ostringstream& out, int which) const;

  /// Handle final guard generation for streams that are not included in the
  /// BE_BuiltinGlobalData structure.
  virtual void postprocess_guard_end(const std::string& macro, std::ostringstream& out, int which) const;
};

#endif /* OPENDDS_IDL_LANGUAGE_MAPPING_H */
