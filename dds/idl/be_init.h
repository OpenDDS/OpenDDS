#ifndef OPENDDS_IDL_BE_INIT_H
#define OPENDDS_IDL_BE_INIT_H

// This is an extension of be_extern.h
// It supplies global functions for operations that apply to all
// loaded plugins.

#include <ace/OS_NS_string.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

// Used to register a backend with the idl compiler.
bool BE_register(const ACE_TCHAR* dllname, const ACE_TCHAR* allocator);

void BE_destroy();
void BE_parse_args(long& i, char** av);

void BE_prep_be_arg(char* arg);
void BE_arg_post_proc();
void BE_usage();

#endif /* OPENDDS_IDL_BE_INIT_H */
