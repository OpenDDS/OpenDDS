#include <gtest/gtest.h>

#include <dds/DCPS/debug.h>

#include <dds/OpenDDSConfigWrapper.h>

#if OPENDDS_CONFIG_SECURITY
#  include <dds/DCPS/security/OpenSSL_init.h>
#endif

#include <ace/Init_ACE.h>
#include <ace/Arg_Shifter.h>
#include <ace/Log_Msg.h>

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);

  ACE_Arg_Shifter_T<char> args(argc, argv);
  args.ignore_arg(); // argv[0] is the program name
  while (args.is_anything_left()) {
    const char* arg = 0;
    if ((arg = args.get_the_parameter("-DCPSLogLevel")) != 0) {
      OpenDDS::DCPS::log_level.set_from_string(arg);
      args.consume_arg();
    } else {
      args.ignore_arg();
    }
  }

#if OPENDDS_CONFIG_SECURITY
  openssl_init();
#endif
  ACE::init();
  int result = RUN_ALL_TESTS();
  ACE::fini();
  return result;
}
