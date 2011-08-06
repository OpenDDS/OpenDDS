/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef MESSENGER_TEST_ARGS_H
#define MESSENGER_TEST_ARGS_H

#include <dds/DCPS/transport/framework/TransportRegistry.h>

#include <ace/Argv_Type_Converter.h>
#include <ace/Get_Opt.h>
#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>

inline int
parse_args(int argc, ACE_TCHAR *argv[])
{
  ACE_Get_Opt get_opts(argc, argv, ACE_TEXT("t:"));

  std::string transport_type;
  int c;
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
    case '?':
    default:
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("usage: %s [-t transport]\n"), argv[0]),
                       -1);
    }
  }

  if (!transport_type.empty()) {
    OpenDDS::DCPS::TransportRegistry* reg = TheTransportRegistry;
    OpenDDS::DCPS::TransportConfig_rch cfg = reg->create_config("myconfig");
    cfg->instances_.push_back(reg->create_inst("myinst", transport_type));
    reg->global_config(cfg);
  }

  return 0;
}


#endif
