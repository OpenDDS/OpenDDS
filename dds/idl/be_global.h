/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_IDL_BE_GLOBAL_H
#define OPENDDS_IDL_BE_GLOBAL_H

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
  // Constructor.

  virtual ~BE_GlobalData();
  // Destructor.

  // Data accessors.

  const char* holding_scope_name() const;

  void destroy();
  // Cleanup function.

  const char* filename() const;
  void filename(const char* fname);

  bool do_included_files() const;

  ACE_CString spawn_options();
  // Command line passed to ACE_Process::spawn. Different
  // implementations in IDL and IFR backends.

  void parse_args(long& i, char** av);
  // Parse args that affect the backend.

  void prep_be_arg(char* s);
  // Special BE arg call factored out of DRV_args.

  void arg_post_proc();
  // Checks made after parsing args.

  void usage() const;
  // Display usage of BE-specific options.

  AST_Generator* generator_init();
  // Create an AST node generator.

  void open_streams(const char* filename);
  void close_streams();

  std::ostringstream header_, impl_, idl_;
  ACE_CString header_name_, impl_name_, idl_name_;

  ///print message to all open streams
  void multicast(const char* message);

  enum stream_enum_t {
    STREAM_H, STREAM_CPP, STREAM_IDL
  };

  void reset_includes();

  void add_include(const char* file, stream_enum_t which = STREAM_H);

  ACE_CString get_include_block(stream_enum_t which);

  ACE_CString export_macro() const;
  void export_macro(const ACE_CString& str);

  ACE_CString export_include() const;
  void export_include(const ACE_CString& str);

  ACE_CString pch_include() const;
  void pch_include(const ACE_CString& str);

  static bool writeFile(const char* fileName, const std::string &content);

private:
  const char* filename_;
  // Name of the IDL file we are processing.

  bool do_server_side_;

  ACE_CString export_macro_, export_include_, pch_include_;
};

class BE_Comment_Guard {
public:

  BE_Comment_Guard(const char* type, const char* name);
  ~BE_Comment_Guard();

private:
  const char *type_, *name_;
};

#endif /* OPENDDS_IDL_BE_GLOBAL_H */
