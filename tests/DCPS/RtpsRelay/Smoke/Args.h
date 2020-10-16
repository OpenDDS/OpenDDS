/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef RTPSRELAY_SMOKE_TEST_ARGS_H
#define RTPSRELAY_SMOKE_TEST_ARGS_H

#include <ace/Argv_Type_Converter.h>
#include <ace/Get_Opt.h>
#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>

#include <iostream>
#include <cstdlib>

extern bool check_lease_recovery;
extern bool expect_unmatch;

inline int
parse_args(int argc, ACE_TCHAR *argv[])
{
  ACE_Get_Opt get_opts(argc, argv, ACE_TEXT("le"));

  int c;
  while ((c = get_opts()) != -1) {
    switch (c) {
    case 'l':
      check_lease_recovery = true;
      break;
    case 'e':
      expect_unmatch = true;
      break;
    case '?':
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("usage: %s [-le]\n"), argv[0]), EXIT_FAILURE);
    }
  }

  return EXIT_SUCCESS;
}

#endif
