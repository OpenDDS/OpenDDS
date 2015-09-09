#include "SimpleTransportClient.h"

#include "dds/DCPS/transport/framework/TransportRegistry.h"
#include "dds/DCPS/transport/tcp/TcpInst.h"
#ifdef ACE_AS_STATIC_LIBS
#include "dds/DCPS/transport/tcp/Tcp.h"
#endif

#include <ace/SOCK_Acceptor.h>

class DDS_TEST
{
public:
  DDS_TEST()
  {
  }
  ACE_INET_Addr& local_address(OpenDDS::DCPS::TcpInst* tcp_inst) {
    return tcp_inst->local_address_;
  }
};

bool testConnectionErrorHandling()
{
  // Open an acceptor to force an existing port number to be used.
  // Use 0 to indicate an available port should be selected.
  u_short portNum = 0;
  ACE_INET_Addr addr(portNum, ACE_LOCALHOST);
  ACE_SOCK_Acceptor acceptor;
  if (-1 == acceptor.open(addr)) {
    ACE_ERROR_RETURN ((LM_ERROR, ACE_TEXT("%N:%l: Failed to open acceptor to ")
                                 ACE_TEXT("obtain a used port number")), false);
  }
  acceptor.get_local_addr(addr);
  portNum = addr.get_port_number();

  OpenDDS::DCPS::TransportInst_rch inst =
    TheTransportRegistry->create_inst("tcp1", "tcp");
  OpenDDS::DCPS::TcpInst_rch tcp_inst =
    OpenDDS::DCPS::dynamic_rchandle_cast<OpenDDS::DCPS::TcpInst>(inst);

  DDS_TEST test;
  acceptor.get_local_addr(test.local_address(tcp_inst.in()));

  OpenDDS::DCPS::TransportConfig_rch cfg =
    TheTransportRegistry->create_config("cfg");
  cfg->instances_.push_back(inst);

  TheTransportRegistry->global_config(cfg);

  SimpleTransportClient transportClient;
  transportClient.exceptionThrown = false;
  transportClient.enable();
  if (transportClient.exceptionThrown) {
    ACE_DEBUG ((LM_INFO, ACE_TEXT("%N:%l: Expected exception thrown. ")
                ACE_TEXT("Any previous error messages about transport ")
                ACE_TEXT("configuration can be ignored.\n")));
    return true;
  } else {
    ACE_ERROR ((LM_ERROR, ACE_TEXT("(%N:%l: ERROR: Exception was expected to ")
                          ACE_TEXT("be thrown but was not.\n")));
    return false;
  }
}

int ACE_TMAIN (int /*argc*/, ACE_TCHAR* /*argv*/[])
{
  const bool result = testConnectionErrorHandling();
  TheTransportRegistry->release();
  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
