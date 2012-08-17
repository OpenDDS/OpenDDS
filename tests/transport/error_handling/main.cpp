#include "SimpleTransportClient.h"

#include "dds/DCPS/transport/framework/TransportRegistry.h"
#include "dds/DCPS/transport/tcp/TcpInst.h"

#include <ace/SOCK_Acceptor.h>

bool testConnectionErrorHandling()
{
  // Open an acceptor to force an existing port number to be used.
  u_short portNum = 0; // Use 0 to indicate an unavailable port should be selected.
  ACE_INET_Addr addr(portNum, ACE_LOCALHOST);
  ACE_SOCK_Acceptor acceptor;
  if (-1 == acceptor.open(addr))
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT("%N:%l: Failed to open acceptor to obtain a used port number")), false);
  acceptor.get_local_addr(addr);
  portNum =  addr.get_port_number();

  OpenDDS::DCPS::TransportInst_rch inst = TheTransportRegistry->create_inst("tcp1", "tcp");
  OpenDDS::DCPS::TcpInst_rch tcp_inst =
    OpenDDS::DCPS::dynamic_rchandle_cast<OpenDDS::DCPS::TcpInst>(inst);

  acceptor.get_local_addr(tcp_inst->local_address_);
  
  OpenDDS::DCPS::TransportConfig_rch cfg = TheTransportRegistry->create_config("cfg");
  cfg->instances_.push_back(inst);

  TheTransportRegistry->global_config(cfg);

  SimpleTransportClient transportClient;
  transportClient.exceptionThrown = false;
  transportClient.enable();
  if (transportClient.exceptionThrown)
    {
      ACE_DEBUG ((LM_INFO, ACE_TEXT("%N:%l: Expected exception thrown. ") 
                  ACE_TEXT("Any previous error messages about transport configuration can be ignored.\n")));
      return true;
    } else {
      ACE_ERROR ((LM_ERROR, ACE_TEXT("(%N:%l: ERROR: Exception was expected to be thrown but was not.\n")));
      return false;
    }
}

int
ACE_TMAIN (int /*argc*/, ACE_TCHAR */*argv*/[])
{
  bool passes = testConnectionErrorHandling();

  if (passes) 
    {
      return EXIT_SUCCESS;
    } else {
      return EXIT_FAILURE;
  }
}
