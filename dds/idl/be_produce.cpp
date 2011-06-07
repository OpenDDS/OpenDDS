/*

COPYRIGHT

Copyright 1992, 1993, 1994 Sun Microsystems, Inc.  Printed in the United
States of America.  All Rights Reserved.

This product is protected by copyright and distributed under the following
license restricting its use.

The Interface Definition Language Compiler Front End (CFE) is made
available for your use provided that you include this license and copyright
notice on all media and documentation and the software program in which
this product is incorporated in whole or part. You may copy and extend
functionality (but may not remove functionality) of the Interface
Definition Language CFE without charge, but you are not authorized to
license or distribute it to anyone else except as part of a product or
program developed by you or with the express written consent of Sun
Microsystems, Inc. ("Sun").

The names of Sun Microsystems, Inc. and any of its subsidiaries or
affiliates may not be used in advertising or publicity pertaining to
distribution of Interface Definition Language CFE as permitted herein.

This license is effective until terminated by Sun for failure to comply
with this license.  Upon termination, you shall destroy or return all code
and documentation for the Interface Definition Language CFE.

INTERFACE DEFINITION LANGUAGE CFE IS PROVIDED AS IS WITH NO WARRANTIES OF
ANY KIND INCLUDING THE WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS
FOR A PARTICULAR PURPOSE, NONINFRINGEMENT, OR ARISING FROM A COURSE OF
DEALING, USAGE OR TRADE PRACTICE.

INTERFACE DEFINITION LANGUAGE CFE IS PROVIDED WITH NO SUPPORT AND WITHOUT
ANY OBLIGATION ON THE PART OF Sun OR ANY OF ITS SUBSIDIARIES OR AFFILIATES
TO ASSIST IN ITS USE, CORRECTION, MODIFICATION OR ENHANCEMENT.

SUN OR ANY OF ITS SUBSIDIARIES OR AFFILIATES SHALL HAVE NO LIABILITY WITH
RESPECT TO THE INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY
INTERFACE DEFINITION LANGUAGE CFE OR ANY PART THEREOF.

IN NO EVENT WILL SUN OR ANY OF ITS SUBSIDIARIES OR AFFILIATES BE LIABLE FOR
ANY LOST REVENUE OR PROFITS OR OTHER SPECIAL, INDIRECT AND CONSEQUENTIAL
DAMAGES, EVEN IF SUN HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.

Use, duplication, or disclosure by the government is subject to
restrictions as set forth in subparagraph (c)(1)(ii) of the Rights in
Technical Data and Computer Software clause at DFARS 252.227-7013 and FAR
52.227-19.

Sun, Sun Microsystems and the Sun logo are trademarks or registered
trademarks of Sun Microsystems, Inc.

SunSoft, Inc.
2550 Garcia Avenue
Mountain View, California  94043

NOTE:

SunOS, SunSoft, Sun, Solaris, Sun Microsystems or the Sun logo are
trademarks or registered trademarks of Sun Microsystems, Inc.

*/

// be_produce.cpp - Produce the work of the BE

#include "global_extern.h"

#include "be_extern.h"
#include "dds_visitor.h"

#include "ast_root.h"
#include "utl_string.h"

#include "ace/OS_NS_strings.h"
#include "ace/OS_NS_sys_time.h"
#include "ace/OS_NS_unistd.h"

#include "../Version.h"
#include "ace/Version.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <limits>
#include <cassert>

using namespace std;

// Clean up before exit, whether successful or not.
// Need not be exported since it is called only from this file.
void
BE_cleanup()
{
  idl_global->destroy();
}

// Abort this run of the BE.
void
BE_abort()
{
  ACE_ERROR((LM_ERROR, ACE_TEXT("Fatal Error - Aborting\n")));
  BE_cleanup();
  ACE_OS::exit(1);
}

namespace {

/// generate a macro name for the #ifndef header-double-include protector
string to_macro(const char* fn)
{
  string ret = "OPENDDS_IDL_GENERATED_";

  for (size_t i = 0; i < strlen(fn); ++i) {
    if (isalnum(fn[i])) {
      ret += static_cast<char>(toupper(fn[i]));
    } else if (ret[ret.size() - 1] != '_') {
      ret += '_';
    }
  }

  // Add some random characters since two files of the same name (in different
  // directories) could be used in the same translation unit.  The algorithm
  // for randomness comes from TAO_IDL's implementation.

  const size_t NUM_CHARS = 6;

  const ACE_Time_Value now = ACE_OS::gettimeofday();
  ACE_UINT64 msec;
  now.msec(msec);

  msec += ACE_OS::getpid() + (size_t) ACE_OS::thr_self();

  unsigned int seed = static_cast<unsigned int>(msec);
#ifdef max
#undef max
#endif
  const float MAX_VAL = static_cast<float>(numeric_limits<char>::max());
  const float coefficient = static_cast<float>(MAX_VAL / (RAND_MAX + 1.0f));

  if (ret[ret.size() - 1] != '_') ret += '_';

  for (unsigned int n = 0; n < NUM_CHARS; ++n) {
    char r;
    do {
      r = static_cast<char>(coefficient * ACE_OS::rand_r(&seed));
    } while (!isalnum(r));

    ret += static_cast<char>(toupper(r));
  }

  return ret;
}

/// change *.cpp to *.h
string to_header(const char* cpp_name)
{
  size_t len = strlen(cpp_name);
  assert(len >= 5 && 0 == ACE_OS::strcasecmp(cpp_name + len - 4, ".cpp"));
  string base_name(cpp_name, len - 4);
  return base_name + ".h";
}

void postprocess(const char* fn, ostringstream& content,
                 BE_GlobalData::stream_enum_t which)
{
  ostringstream out;

  if (which == BE_GlobalData::STREAM_H) {
    out << "/* -*- C++ -*- */\n";
  }

  out << "/* Generated by " << idl_global->prog_name()
      << " version " DDS_VERSION " (ACE version " ACE_VERSION
      << ") running on input file " << idl_global->main_filename()->get_string()
      << " */\n";

  //  if .h add #ifndef...#define
  string macrofied;

  if (which == BE_GlobalData::STREAM_H) {
    macrofied = to_macro(fn);
    out << "#ifndef " << macrofied << "\n#define " << macrofied << '\n';
    string taoheader = be_global->header_name_.c_str();
    taoheader.replace(taoheader.find("TypeSupportImpl.h"), 17, "C.h");
    out << "#include \"" << be_global->tao_inc_pre_ << taoheader << "\"\n";

  } else if (which == BE_GlobalData::STREAM_CPP) {
    ACE_CString pch = be_global->pch_include();
    if (pch.length()) {
      out << "#include \"" << pch << "\"\n";
    }
    if (be_global->java_arg().length() == 0) {
      string header = to_header(fn);
      out << "#include \"" << header << "\"\n\n";
    } else {
      out << "#include \"" << be_global->header_name_.c_str() << "\"\n\n";
    }

  } else { //STREAM_IDL
    out << "#include \"" << be_global->filename() << "\"\n\n";
  }

  out << be_global->get_include_block(which);

  out << content.str();

  //  if .h add #endif
  if (which == BE_GlobalData::STREAM_H) {
    out << "#endif /* " << macrofied << " */\n";
  }

  if (!BE_GlobalData::writeFile(fn, out.str())) {
    BE_abort();  //error message already printed
  }
}

} // namespace

// Do the work of this BE. This is the starting point for code generation.
void
BE_produce()
{
  //search for #includes in the IDL, add them as #includes in the stubs/skels
  const char* idl_fn = idl_global->main_filename()->get_string();

  ifstream idl(idl_fn);
  const size_t buffer_sz = 512;
  char buffer[buffer_sz];

  while (idl) {
    idl.getline(buffer, buffer_sz);

    if (0 == strncmp("#include", buffer, 8)) { //FUTURE: account for comments?
      string inc(buffer + 8);
      size_t delim1 = inc.find_first_of("<\"");
      size_t delim2 = inc.find_first_of(">\"", delim1 + 1);
      string included(inc, delim1 + 1, delim2 - delim1 - 1);
      size_t len = included.size();
      string base_name;

      if (len >= 5 &&
          0 == ACE_OS::strcasecmp(included.c_str() + len - 4, ".idl")) {
        base_name.assign(included.c_str(), len - 4);

      } else if (len >= 6 &&
                 0 == ACE_OS::strcasecmp(included.c_str() + len - 5, ".pidl")) {
        base_name.assign(included.c_str(), len - 5);

      } else {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%N:%l) BE_produce - included ")
                   ACE_TEXT("file must end in .idl or .pidl\n")));
        BE_abort();
      }

      string stb_inc = base_name + "C.h";
      if (stb_inc != "tao/orbC.h") {
        be_global->add_include(stb_inc.c_str());
      }

    }
  }

  idl.close();

  be_global->open_streams(idl_fn);

  AST_Decl* d = idl_global->root();
  AST_Root* root = AST_Root::narrow_from_decl(d);

  if (root == 0) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%N:%l) BE_produce - ")
               ACE_TEXT("No Root\n")));

    BE_abort();
  }

  be_global->set_inc_paths(idl_global->idl_flags());

  const bool java_ts_only = be_global->java_arg().length() > 0;

  dds_visitor visitor(d, java_ts_only);

  if (root->ast_accept(&visitor) == -1) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%N:%l) BE_produce -")
               ACE_TEXT(" failed to accept adding visitor\n")));
    BE_abort();
  }

  if (!java_ts_only) {
    postprocess(be_global->header_name_.c_str(),
                be_global->header_, BE_GlobalData::STREAM_H);
    if (!be_global->suppress_idl()) {
      postprocess(be_global->idl_name_.c_str(),
                  be_global->idl_, BE_GlobalData::STREAM_IDL);
    }
  }

  postprocess(be_global->impl_name_.c_str(),
              be_global->impl_, BE_GlobalData::STREAM_CPP);

  BE_cleanup();
}
