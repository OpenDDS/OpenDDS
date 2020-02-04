/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef IDL2JNI_BE_GLOBAL_H
#define IDL2JNI_BE_GLOBAL_H

#include "ace/SString.h"

#include <string>
#include <sstream>

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

  const char *holding_scope_name() const;

  void destroy();
  // Cleanup function.

  const char *filename() const;
  void filename(const char *fname);

  bool do_included_files() const;

  bool do_server_side() const;
  void do_server_side(bool val);

  ACE_CString spawn_options();
  // Command line passed to ACE_Process::spawn. Different
  // implementations in IDL and IFR backends.

  void parse_args(long &i, char **av);
  // Parse args that affect the backend.

  void open_streams(const char *filename);
  void close_streams();

  std::ostringstream stub_header_, stub_impl_, skel_header_, skel_impl_;
  ACE_CString stub_header_name_, stub_impl_name_, skel_header_name_,
    skel_impl_name_, tao_inc_pre_;

  ///print message to all open streams
  void multicast(const char *message);

  enum stream_enum_t {
    STUB_H, STUB_CPP, SKEL_H, SKEL_CPP
  };

  void reset_includes();

  void add_include(const char *file, stream_enum_t which = STUB_H);

  ACE_CString get_include_block(stream_enum_t which);

  ACE_CString stub_export_macro() const;
  void stub_export_macro(const ACE_CString &str);

  ACE_CString stub_export_include() const;
  void stub_export_include(const ACE_CString &str);

  ACE_CString skel_export_macro() const;
  void skel_export_macro(const ACE_CString &str);

  ACE_CString skel_export_include() const;
  void skel_export_include(const ACE_CString &str);

  ACE_CString native_lib_name() const;
  void native_lib_name(const ACE_CString &str);

  static bool writeFile(const char *fileName, const std::string &content);

  /**
   * Give a warning that looks like one from tao_idl
   */
  void warning(const char* msg, const char* filename = 0, unsigned lineno = 0);

  /**
   * Give an error that looks like one from tao_idl
   */
  void error(const char* msg, const char* filename = 0, unsigned lineno = 0);

private:
  const char *filename_;
  // Name of the IDL file we are processing.

  bool do_server_side_;

  ACE_CString stub_export_macro_, stub_export_include_,
  skel_export_macro_, skel_export_include_, native_lib_name_;
};

class BE_Comment_Guard {
public:

  BE_Comment_Guard(const char *type, const char *name);
  ~BE_Comment_Guard();

private:
  const char *type_, *name_;
};

#endif /* IDL2JNI_BE_GLOBAL_H */
