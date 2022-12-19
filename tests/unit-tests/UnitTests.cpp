#include <gtest/gtest.h>

#include <dds/DCPS/debug.h>
#ifdef OPENDDS_SECURITY
#  include <dds/DCPS/security/OpenSSL_init.h>
#endif

#include <ace/Init_ACE.h>
#include <ace/Arg_Shifter.h>
#include <ace/Log_Msg.h>

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);

  ACE_Arg_Shifter args(argc, argv);
  args.ignore_arg(); // argv[0] is the program name
  while (args.is_anything_left()) {
    const ACE_TCHAR* arg = 0;
    if ((arg = args.get_the_parameter(ACE_TEXT("-DCPSLogLevel"))) != 0) {
      OpenDDS::DCPS::log_level.set_from_string(ACE_TEXT_ALWAYS_CHAR(arg));
      args.consume_arg();
    } else {
      args.ignore_arg();
    }
  }

#ifdef OPENDDS_SECURITY
  openssl_init();
#endif
  ACE::init();
  int result = RUN_ALL_TESTS();
  ACE::fini();
  return result;
}
