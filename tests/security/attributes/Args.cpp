#include "Args.h"

#include <dds/DCPS/transport/framework/TransportRegistry.h>

#include <dds/DCPS/transport/framework/TransportConfig.h>
#include <dds/DCPS/transport/framework/TransportInst.h>

#include <ace/Argv_Type_Converter.h>
#include "ace/Arg_Shifter.h"
#include <ace/Get_Opt.h>
#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>
#include <iostream>

namespace SecurityAttributes
{

Args::Args()
 : auth_ca_file_("file:../../security/certs/opendds_identity_ca_cert.pem")
 , perm_ca_file_("file:../../security/certs/opendds_identity_ca_cert.pem")
 , id_cert_file_("file:identity_cert.pem")
 , id_key_file_("file:identity_private_key.pem")
 , governance_file_("file:governance.p7s")
 , permissions_file_("file:permissions.p7s")
 , domain_(0)
 , topic_name_("OD_OA_OM_OD")
 , reliable_(false)
 , wait_for_acks_(false)
 , num_messages_(DEFAULT_NUM_MESSAGES)
 , expected_result_(0)
 , timeout_(0)
 , extra_space(0)
{
}

namespace {
std::string operator+(const std::string& str, const wchar_t* app) {
  return str + ACE_Wide_To_Ascii(app).char_rep();
}
}

//static
int Args::parse_args(int argc, ACE_TCHAR *argv[], Args& args)
{
  ACE_Arg_Shifter arg_shifter(argc, argv);

  while (arg_shifter.is_anything_left()) {
    const ACE_TCHAR *currentArg = 0;

    if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-IdentityCA"))) != 0) {
      args.auth_ca_file_ = std::string("file:") + currentArg;
      arg_shifter.consume_arg();
    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-PermissionsCA"))) != 0) {
      args.perm_ca_file_ = std::string("file:") + currentArg;
      arg_shifter.consume_arg();
    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-Identity"))) != 0) {
      args.id_cert_file_ = std::string("file:") + currentArg;
      arg_shifter.consume_arg();
    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-PrivateKey"))) != 0) {
      args.id_key_file_ = std::string("file:") + currentArg;
      arg_shifter.consume_arg();
    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-Governance"))) != 0) {
      args.governance_file_ = std::string("file:") + currentArg;
      arg_shifter.consume_arg();
    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-Permissions"))) != 0) {
      args.permissions_file_ = std::string("file:") + currentArg;
      arg_shifter.consume_arg();
    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-Domain"))) != 0) {
      args.domain_ = ACE_OS::atoi(currentArg);
      arg_shifter.consume_arg();
    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-ExtraSpace"))) != 0) {
       args.extra_space = ACE_OS::atoi(currentArg);
       arg_shifter.consume_arg();
    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-Topic"))) != 0) {
      args.topic_name_ = std::string("") + currentArg;
      arg_shifter.consume_arg();
    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-Expected"))) != 0) {
      if (currentArg != NULL && *currentArg == '~') {
        args.expected_result_ = -1 * ACE_OS::atoi(++currentArg);
      } else {
        args.expected_result_ = ACE_OS::atoi(currentArg);
      }
      arg_shifter.consume_arg();
    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-Timeout"))) != 0) {
      if (currentArg != NULL && *currentArg == '~') {
        args.timeout_ = -1 * ACE_OS::atoi(++currentArg);
      } else {
        args.timeout_ = ACE_OS::atoi(currentArg);
      }
      arg_shifter.consume_arg();
    } else {
      arg_shifter.ignore_arg();
    }
  }

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
      args.reliable_ = true;
      break;
    case 'w':
      args.wait_for_acks_ = true;
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

  if (thread_per_connection) {
    OpenDDS::DCPS::TransportConfig_rch config =
      TheTransportRegistry->fix_empty_default();
    if (config.in() == 0) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("no default config\n"), argv[0]),
                       -2);
    }
    else if (config->instances_.size() < 1) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("no instances on default config\n"), argv[0]),
                       -3);
    }
    else if (config->instances_.size() > 1) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("too many instances on default config, using first\n"), argv[0]));
    }
    OpenDDS::DCPS::TransportInst_rch inst = *(config->instances_.begin());
    inst->thread_per_connection_ = true;
  }

  return 0;
}

}

