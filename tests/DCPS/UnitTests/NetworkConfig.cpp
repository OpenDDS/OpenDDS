#include "ace/OS_main.h"
#include <dds/DCPS/NetworkConfigModifier.h>

int ACE_TMAIN(int, ACE_TCHAR*[])
{
#ifdef OPENDDS_NETWORK_CONFIG_MODIFIER
  NetworkConfigModifier mod;
  mod.add_interface("eth0");
  mod.add_address("eth0", ACE_INET_Addr(42));
  mod.remove_address("eth0", ACE_INET_Addr(42));
  mod.remove_interface("eth0");
#endif

  return 0;
}
