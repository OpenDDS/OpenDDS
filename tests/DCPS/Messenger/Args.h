/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef MESSENGER_TEST_ARGS_H
#define MESSENGER_TEST_ARGS_H

#include <dds/DCPS/transport/framework/TransportRegistry.h>

#include <dds/DCPS/transport/framework/TransportConfig.h>
#include <dds/DCPS/transport/framework/TransportInst.h>

#include <ace/Argv_Type_Converter.h>
#include <ace/Get_Opt.h>
#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>

#include <iostream>
#include <cstdlib>

const int num_messages = 40;
extern bool reliable;
extern bool wait_for_acks;

inline int
parse_args(int argc, ACE_TCHAR *argv[])
{
  ACE_Get_Opt get_opts(argc, argv, ACE_TEXT("t:prw"));

  OPENDDS_STRING transport_type;
  int c;
  bool thread_per_connection = false;
  while ((c = get_opts()) != -1) {
    switch (c) {
    case 't':

      if (ACE_OS::strcmp(get_opts.opt_arg(), ACE_TEXT("udp")) == 0) {
        transport_type = "udp";

      } else if (ACE_OS::strcmp(get_opts.opt_arg(), ACE_TEXT("multicast")) == 0) {
        transport_type = "multicast";

      } else if (ACE_OS::strcmp(get_opts.opt_arg(), ACE_TEXT("tcp")) == 0) {
        transport_type = "tcp";
      }

      break;
    case 'p':
      thread_per_connection = true;
      break;
    case 'r':
      reliable = true;
      break;
    case 'w':
      wait_for_acks = true;
      break;
    case '?':
    default:
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("usage: %s [-t transport]\n"), argv[0]),
                       EXIT_FAILURE);
    }
  }

  if (!transport_type.empty()) {
    OpenDDS::DCPS::TransportRegistry* reg = TheTransportRegistry;
    OpenDDS::DCPS::TransportConfig_rch cfg = reg->create_config("myconfig");
    cfg->instances_.push_back(reg->create_inst("myinst", transport_type));
    reg->global_config(cfg);
  }

  if (thread_per_connection) {
    OpenDDS::DCPS::TransportConfig_rch config =
      TheTransportRegistry->fix_empty_default();
    if (config.in() == 0) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("no default config\n"), argv[0]),
                       EXIT_FAILURE);
    }
    else if (config->instances_.size() < 1) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("no instances on default config\n"), argv[0]),
                       EXIT_FAILURE);
    }
    else if (config->instances_.size() > 1) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("too many instances on default config, using first\n"), argv[0]));
    }
    OpenDDS::DCPS::TransportInst_rch inst = *(config->instances_.begin());
    inst->thread_per_connection_ = true;
  }

  return EXIT_SUCCESS;
}


#endif
