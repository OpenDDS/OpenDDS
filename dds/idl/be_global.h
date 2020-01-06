/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_IDL_BE_GLOBAL_H
#define OPENDDS_IDL_BE_GLOBAL_H

#include "annotations.h"

#include <utl_scoped_name.h>
#include <idl_defines.h>
#ifndef TAO_IDL_HAS_ANNOTATIONS
#  error "Annotation support in tao_idl is required, please use a newer version of TAO"
#endif

#include <ace/SString.h>

#include <string>
#include <sstream>
#include <set>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class AST_Generator;
class AST_Decl;
class UTL_Scope;
class AST_Structure;
class AST_Field;
class AST_Union;
class AST_Annotation_Decl;

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

  ACE_CString spawn_options();
  // Command line passed to ACE_Process::spawn. Different
  // implementations in IDL and IFR backends.

  void parse_args(long& i, char** av);
  // Parse args that affect the backend.

  void open_streams(const char* filename);

  std::ostringstream header_, impl_, idl_, itl_, facets_header_, facets_impl_,
    lang_header_;
  ACE_CString header_name_, impl_name_, idl_name_, itl_name_,
    facets_header_name_, facets_impl_name_, lang_header_name_,
    output_dir_, tao_inc_pre_;

  ///print message to all open streams
  void multicast(const char* message);

  enum stream_enum_t {
    STREAM_H, STREAM_CPP, STREAM_IDL, STREAM_ITL,
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

  const std::set<std::string>& cpp_includes() const;
  void add_cpp_include(const std::string& str);

  bool java() const;
  void java(bool b);

  bool no_default_gen() const;
  void no_default_gen(bool b);

  bool itl() const;
  void itl(bool b);

  bool v8() const;
  void v8(bool b);

  bool rapidjson() const;
  void rapidjson(bool b);

  bool face_ts() const;
  void face_ts(bool b);

  ACE_CString java_arg() const;
  void java_arg(const ACE_CString& str);

  enum LanguageMapping {
    LANGMAP_NONE, ///< Don't generate, let tao_idl handle it
    LANGMAP_FACE_CXX, ///< Generate C++ language mapping from FACE spec
    LANGMAP_SP_CXX, ///< Generate C++ language mapping for Safety Profile
    LANGMAP_CXX11, ///< Generate OMG IDL-to-C++11
  };

  LanguageMapping language_mapping() const;
  void language_mapping(LanguageMapping lm);

  ACE_CString sequence_suffix() const;
  void sequence_suffix(const ACE_CString& str);

  bool suppress_idl() const { return suppress_idl_; }
  bool suppress_typecode() const { return suppress_typecode_; }

  static bool writeFile(const char* fileName, const std::string &content);

  /**
   * Based on annotations and global_default_nested_, determine if a type is a
   * topic type and needs type support.
   *
   * Does not check for specific types of types (struct vs array).
   */
  bool is_topic_type(AST_Decl* node);

  /**
   * Nested property of the root module. Assuming there are no annotations, all
   * potential topic types inherit this value. True by default unless
   * --no-default-nested was passed.
   */
  bool root_default_nested() const;

  /**
   * If node has the key annotation, this sets value to the key annotation
   * value and returns true, else this sets value to false and returns false.
   */
  bool check_key(AST_Field* node, bool& value);

  /**
   * Check if the discriminator in a union has been declared a key.
   */
  bool has_key(AST_Union* node);

  /**
   * Give a warning that looks like one from tao_idl
   */
  void warning(const char* msg, const char* filename = 0, unsigned lineno = 0);

  /**
   * Give an error that looks like one from tao_idl
   */
  void error(const char* msg, const char* filename = 0, unsigned lineno = 0);

  /**
   * Wrapper around built-in annotations, see annotations.h
   */
  BuiltinAnnotations builtin_annotations_;

  /**
   * If true, warn about #pragma DCPS_DATA_TYPE
   */
  bool warn_about_dcps_data_type();

private:
  /// Name of the IDL file we are processing.
  const char* filename_;

  bool java_, suppress_idl_, suppress_typecode_,
    no_default_gen_, generate_itl_, generate_v8_,
    generate_rapidjson_, face_ts_;

  ACE_CString export_macro_, export_include_,
    versioning_name_, versioning_begin_, versioning_end_,
    pch_include_, java_arg_, seq_;
  std::set<std::string> cpp_includes_;

  LanguageMapping language_mapping_;

  bool root_default_nested_;
  bool warn_about_dcps_data_type_;

  bool is_nested(AST_Decl* node);
  bool is_default_nested(UTL_Scope* scope);
};

class BE_Comment_Guard {
public:

  BE_Comment_Guard(const char* type, const char* name);
  ~BE_Comment_Guard();

private:
  const char *type_, *name_;
};

#endif /* OPENDDS_IDL_BE_GLOBAL_H */
