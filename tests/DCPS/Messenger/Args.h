/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef MESSENGER_TEST_ARGS_H
#define MESSENGER_TEST_ARGS_H

#include <dds/DCPS/ArgParsing.h>
#include <dds/DCPS/transport/framework/TransportRegistry.h>
#include <dds/DCPS/transport/framework/TransportConfig.h>
#include <dds/DCPS/transport/framework/TransportInst.h>

#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>

#include <iostream>
#include <cstdlib>

const size_t num_messages = 40;

inline
int parse_args(int argc, ACE_TCHAR* argv[])
{
  using OpenDDS::DCPS::String;
  String transport_type;
  bool thread_per_connection = false;
  {
    using namespace OpenDDS::DCPS::ArgParsing;
    ArgParser arg_parser("");

    Option thread_per_connection_opt(arg_parser, "thread-per-connection",
      "Set thread-per-connection on the transport", thread_per_connection);
    thread_per_connection_opt.add_alias("p");

    OptionAs<StringChoiceValue> transport_opt(arg_parser, "transport",
      "Select a transport type", transport_type, "");
    transport_opt.handler.add_choice("udp", "UDP transport");
    transport_opt.handler.add_choice("multicast", "Multicast transport");
    transport_opt.handler.add_choice("tcp", "TCP transport");
    transport_opt.add_alias("t");

    arg_parser.parse(argc, argv);
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
    if (!config) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: parse_args: no default config\n"));
      return EXIT_FAILURE;
    } else if (config->instances_.size() < 1) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: parse_args: no instances on default config\n"));
      return EXIT_FAILURE;
    } else if (config->instances_.size() > 1) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: parse_args: too many instances on default config, using first\n"));
    }
    OpenDDS::DCPS::TransportInst_rch inst = *(config->instances_.begin());
    inst->thread_per_connection_ = true;
  }

  return EXIT_SUCCESS;
}


#endif
