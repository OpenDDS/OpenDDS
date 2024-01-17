/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_IDL_BE_GLOBAL_H
#define OPENDDS_IDL_BE_GLOBAL_H

#include "annotations.h"

#include <dds/DCPS/XTypes/TypeObject.h>

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
    STREAM_LANG_H,
    STREAM_COUNT
  };

  void reset_includes();

  void add_include(const char* file, stream_enum_t which = STREAM_H);
  void conditional_include(const char* file, stream_enum_t which, const char* condition);

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

  const std::set<std::pair<std::string, std::string> >& cpp_includes() const;
  void add_cpp_include(const std::string& str);

  bool java() const;
  void java(bool b);

  bool no_default_gen() const;
  void no_default_gen(bool b);

  bool filename_only_includes() const;
  void filename_only_includes(bool b);

  bool itl() const;
  void itl(bool b);

  bool value_reader_writer() const;
  void value_reader_writer(bool b);

  bool face_ts() const;
  void face_ts(bool b);

  bool xtypes_complete() const;
  void xtypes_complete(bool b);

  bool old_typeobject_encoding() const { return old_typeobject_encoding_; }
  void old_typeobject_encoding(bool b) { old_typeobject_encoding_ = b; }

  bool old_typeobject_member_order() const { return old_typeobject_member_order_; }
  void old_typeobject_member_order(bool b) { old_typeobject_member_order_ = b; }

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
  bool suppress_xtypes() const { return suppress_xtypes_; }

  static bool writeFile(const char* fileName, const std::string& content);

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
  bool check_key(AST_Decl* node, bool& value) const;

  /**
   * Check if the discriminator in a union has been declared a key.
   */
  bool union_discriminator_is_key(AST_Union* node);

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
  Annotations builtin_annotations_;

  /**
   * If true, warn about #pragma DCPS_DATA_TYPE
   */
  bool warn_about_dcps_data_type();

  ExtensibilityKind extensibility(AST_Decl* node, ExtensibilityKind default_extensibility, bool& has_annotation) const;
  ExtensibilityKind extensibility(AST_Decl* node, ExtensibilityKind default_extensibility) const;
  ExtensibilityKind extensibility(AST_Decl* node) const;
  AutoidKind autoid(AST_Decl* node) const;
  bool id(AST_Decl* node, ACE_CDR::ULong& value) const;
  bool hashid(AST_Decl* node, std::string& value) const;
  bool is_optional(AST_Decl* node) const;
  bool is_must_understand(AST_Decl* node) const;
  bool is_effectively_must_understand(AST_Decl* node) const;
  bool is_key(AST_Decl* node) const;
  bool is_external(AST_Decl* node) const;
  bool is_plain(AST_Decl* node) const;

  TryConstructFailAction try_construct(AST_Decl* node) const;
  TryConstructFailAction sequence_element_try_construct(AST_Sequence* node);
  TryConstructFailAction array_element_try_construct(AST_Array* node);
  TryConstructFailAction union_discriminator_try_construct(AST_Union* node);

#if OPENDDS_HAS_IDL_MAP
  TryConstructFailAction map_key_try_construct(AST_Map* node);
  TryConstructFailAction map_value_try_construct(AST_Map* node);
#endif

  OpenDDS::DataRepresentation data_representations(AST_Decl* node) const;

  OpenDDS::XTypes::MemberId compute_id(AST_Structure* stru, AST_Field* field, AutoidKind auto_id,
    OpenDDS::XTypes::MemberId& member_id);
  OpenDDS::XTypes::MemberId get_id(AST_Field* field);

  bool is_nested(AST_Decl* node);

  bool default_enum_extensibility_zero() const
  {
    return default_enum_extensibility_zero_;
  }

  bool dynamic_data_adapter(AST_Decl* node) const;

  bool special_serialization(AST_Decl* node, std::string& template_name) const;

  void generate_equality(bool flag) { generate_equality_ = flag; }
  bool generate_equality() const { return generate_equality_; }

private:
  /// Name of the IDL file we are processing.
  const char* filename_;

  bool java_, suppress_idl_, suppress_typecode_, suppress_xtypes_,
    no_default_gen_, generate_itl_,
    generate_value_reader_writer_,
    generate_xtypes_complete_, face_ts_, generate_equality_;

  bool filename_only_includes_;

  ACE_CString export_macro_, export_include_,
    versioning_name_, versioning_begin_, versioning_end_,
    pch_include_, java_arg_, sequence_suffix_;
  std::set<std::pair<std::string, std::string> > cpp_includes_;

  LanguageMapping language_mapping_;

  bool root_default_nested_;
  bool warn_about_dcps_data_type_;
  ExtensibilityKind default_extensibility_;
  bool default_enum_extensibility_zero_;
  OpenDDS::DataRepresentation default_data_representation_;
  AutoidKind root_default_autoid_;
  TryConstructFailAction default_try_construct_;
  std::set<std::string> platforms_;
  typedef std::map<AST_Field*, OpenDDS::XTypes::MemberId> MemberIdMap;
  MemberIdMap member_id_map_;
  typedef std::map<OpenDDS::XTypes::MemberId, AST_Field*> MemberIdCollisionMap;
  typedef std::map<AST_Structure*, MemberIdCollisionMap> GlobalMemberIdCollisionMap;
  GlobalMemberIdCollisionMap member_id_collision_map_;
  bool old_typeobject_encoding_;
  bool old_typeobject_member_order_;

  bool is_default_nested(UTL_Scope* scope);
  AutoidKind scoped_autoid(UTL_Scope* scope) const;
};

class BE_Comment_Guard {
public:

  BE_Comment_Guard(const char* type, const char* name);
  ~BE_Comment_Guard();

private:
  const char *type_, *name_;
};

#endif /* OPENDDS_IDL_BE_GLOBAL_H */
