/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_IDL_BE_INIT_H
#define OPENDDS_IDL_BE_INIT_H

// This is an extension of be_extern.h
// It supplies global functions for operations that apply to all
// loaded plugins.
#include "be_interface.h"
#include <ace/OS_NS_string.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

void BE_destroy();
void BE_parse_args(long& i, char** av);

void BE_prep_be_arg(char* arg);
void BE_arg_post_proc();
void BE_usage();

// This is public to facilitate statically linking backend plugins.
typedef BE_Interface* (*interface_instance_t)();

#endif /* OPENDDS_IDL_BE_INIT_H */
