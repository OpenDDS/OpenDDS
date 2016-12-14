/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_IDL_BE_GLOBAL_H
#define OPENDDS_IDL_BE_GLOBAL_H

#include "ace/SString.h"

#include <string>
#include <sstream>
#include <set>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class AST_Generator;

// Defines a class containing all back end global data.

class BE_GlobalData {
public:
  // = TITLE
  //    BE_GlobalData
  //
  // = DESCRIPTION
  //    Storage of global data specific to the compiler back end
  //
  BE_GlobalData();

  virtual ~BE_GlobalData();

  // Data accessors.

  const char* holding_scope_name() const;

  void destroy();
  // Cleanup function.

  const char* filename() const;
  void filename(const char* fname);

  //bool do_included_files() const;

  ACE_CString spawn_options();
  // Command line passed to ACE_Process::spawn. Different
  // implementations in IDL and IFR backends.

  void parse_args(long& i, char** av);
  // Parse args that affect the backend.

  void open_streams(const char* filename);

  std::ostringstream header_, impl_, idl_, ws_config_, itl_, facets_header_, facets_impl_,
    lang_header_;
  ACE_CString header_name_, impl_name_, idl_name_, ws_config_name_, itl_name_,
    facets_header_name_, facets_impl_name_, lang_header_name_,
    output_dir_, tao_inc_pre_;

  ///print message to all open streams
  void multicast(const char* message);

  enum stream_enum_t {
    STREAM_H, STREAM_CPP, STREAM_IDL, STREAM_WS, STREAM_ITL,
    STREAM_FACETS_H, STREAM_FACETS_CPP,
    STREAM_LANG_H
  };

  void reset_includes();

  void add_include(const char* file, stream_enum_t which = STREAM_H);

  /// Called to indicate that OpenDDS marshaling (serialization) code for the
  /// current file will depend on marshaling code generated for the indicated
  /// file.  For example, if the current file is A.idl and it contains a struct
  /// which has a field of type B, defined in B.idl, the full path to B.idl is
  /// passed to this function.
  void add_referenced(const char* file);

  void set_inc_paths(const char* cmdline);
  void add_inc_path(const char* path);

  std::string get_include_block(stream_enum_t which);

  ACE_CString export_macro() const;
  void export_macro(const ACE_CString& str);

  ACE_CString export_include() const;
  void export_include(const ACE_CString& str);

  ACE_CString versioning_name() const;
  void versioning_name(const ACE_CString& str);

  ACE_CString versioning_begin() const;
  void versioning_begin(const ACE_CString& str);

  ACE_CString versioning_end() const;
  void versioning_end(const ACE_CString& str);

  ACE_CString pch_include() const;
  void pch_include(const ACE_CString& str);

  bool java() const;
  void java(bool b);

  bool v8() const;
  void v8(bool b);

  bool face_ts() const;
  void face_ts(bool b);

  ACE_CString java_arg() const;
  void java_arg(const ACE_CString& str);

  enum LanguageMapping {
    LANGMAP_NONE, ///< Don't generate, let tao_idl handle it
    LANGMAP_FACE_CXX, ///< Generate C++ language mapping from FACE spec
    LANGMAP_SP_CXX ///< Generate C++ language mapping for Safety Profile
  };

  LanguageMapping language_mapping() const;
  void language_mapping(LanguageMapping lm);

  ACE_CString sequence_suffix() const;
  void sequence_suffix(const ACE_CString& str);

  bool suppress_idl() const { return suppress_idl_; }
  bool suppress_typecode() const { return suppress_typecode_; }

  bool generate_wireshark() const { return generate_wireshark_; }
  bool generate_itl() const { return generate_itl_; }

  static bool writeFile(const char* fileName, const std::string &content);

private:
  const char* filename_;
  // Name of the IDL file we are processing.

  bool java_, suppress_idl_, suppress_typecode_,
    generate_wireshark_, generate_itl_, v8_, face_ts_;

  ACE_CString export_macro_, export_include_, versioning_name_, versioning_begin_, versioning_end_, pch_include_, java_arg_, seq_;

  LanguageMapping language_mapping_;
};

class BE_Comment_Guard {
public:

  BE_Comment_Guard(const char* type, const char* name);
  ~BE_Comment_Guard();

private:
  const char *type_, *name_;
};

#endif /* OPENDDS_IDL_BE_GLOBAL_H */
