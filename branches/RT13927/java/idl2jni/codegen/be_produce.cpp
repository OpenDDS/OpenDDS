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
#include "ast_root.h"
#include "utl_string.h"

#include "ace/OS_NS_strings.h"

#include "idl2jni_visitor.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
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
  ACE_ERROR((LM_ERROR,
             ACE_TEXT("Fatal Error - Aborting\n")));

  BE_cleanup();

  ACE_OS::exit(1);
}

namespace {

/// generate a macro name for the #ifndef header-double-include protector
string to_macro(const char *fn)
{
  string ret = "IDL2JNI_GENERATED_";

  for (size_t i = 0; i < strlen(fn); ++i) {
    ret += isalnum(fn[i]) ? toupper(fn[i]) : '_';
  }

  return ret;
}

/// change *.cpp to *.h
string to_header(const char *cpp_name)
{
  size_t len = strlen(cpp_name);
  assert(len >= 5 && 0 == ACE_OS::strcasecmp(cpp_name + len - 4, ".cpp"));
  string base_name(cpp_name, len - 4);
  return base_name + ".h";
}

/// change *J{C,S}.h to *{C,S}.h
string to_tao(const char *c)
{
  size_t len = strlen(c);
  assert(len >= 5 && (0 == ACE_OS::strcasecmp(c + len - 4, "JC.h")
                      || (0 == ACE_OS::strcasecmp(c + len - 4,
                                                  "JS.h"))));
  string base_name(c, len - 4);
  return base_name + c[len - 3] + ".h";
}

/// change *JS.h to *JC.h
string to_stub(const char *fn)
{
  size_t len = strlen(fn);
  assert(len >= 5 && (0 == ACE_OS::strcasecmp(fn + len - 4, "JS.h")));
  string base_name(fn, len - 4);
  return base_name + "JC.h";
}

void postprocess(const char *fn, ostringstream &content,
                 BE_GlobalData::stream_enum_t which)
{
  ostringstream out;

  //  if .h add #ifndef...#define
  string macrofied;

  if (which == BE_GlobalData::STUB_H || which == BE_GlobalData::SKEL_H) {
    macrofied = to_macro(fn);
    out << "/* -*- C++ -*- */\n";
    out << "#ifndef " << macrofied << "\n#define " << macrofied << '\n';
    string taoheader = to_tao(fn);
    out << "#include \"" << be_global->tao_inc_pre_ << taoheader << "\"\n";

  } else if (which == BE_GlobalData::STUB_CPP ||
             which == BE_GlobalData::SKEL_CPP) {
    string header = to_header(fn);
    out << "#include \"" << header
    << "\"\n#include \"idl2jni_runtime.h\"\n\n";
  }

  out << be_global->get_include_block(which);

  if (which == BE_GlobalData::SKEL_H) {
    string stub = to_stub(fn);
    out << "#include \"" << stub << "\"\n";
  }

  //  write content
  out << content.str();

  //  if .h add #endif
  if (which == BE_GlobalData::STUB_H || which == BE_GlobalData::SKEL_H) {
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
  const char *idl_fn = idl_global->filename()->get_string();

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

      string stb_inc = base_name + "JC.h";
      be_global->add_include(stb_inc.c_str());

      if (be_global->do_server_side()) {
        string skl_inc = base_name + "JS.h";
        be_global->add_include(skl_inc.c_str(),
                               BE_GlobalData::SKEL_CPP);
      }
    }
  }

  idl.close();

  be_global->open_streams(idl_fn);

  // Get the root node.
  AST_Decl *d = idl_global->root();
  AST_Root *root = AST_Root::narrow_from_decl(d);

  if (root == 0) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%N:%l) BE_produce - ")
               ACE_TEXT("No Root\n")));

    BE_abort();
  }

  be_global->multicast("/* Generated by ");
  be_global->multicast(idl_global->prog_name());
  be_global->multicast(" running on input file ");
  be_global->multicast(idl_global->filename()->get_string());
  be_global->multicast(" */\n");

  idl2jni_visitor visitor(d);

  if (root->ast_accept(&visitor) == -1) {
    ACE_ERROR((
                LM_ERROR,
                ACE_TEXT("(%N:%l) BE_produce -")
                ACE_TEXT(" failed to accept adding visitor\n")));

    BE_abort();
  }

  postprocess(be_global->stub_header_name_.c_str(),
              be_global->stub_header_, BE_GlobalData::STUB_H);
  postprocess(be_global->stub_impl_name_.c_str(),
              be_global->stub_impl_, BE_GlobalData::STUB_CPP);

  if (be_global->do_server_side()) {
    postprocess(be_global->skel_header_name_.c_str(),
                be_global->skel_header_, BE_GlobalData::SKEL_H);
    postprocess(be_global->skel_impl_name_.c_str(),
                be_global->skel_impl_, BE_GlobalData::SKEL_CPP);
  }

  be_global->close_streams();

  // Clean up.
  BE_cleanup();
}
