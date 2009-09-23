#include "TcpPublisher.h"
#include "TestException.h"
#include "ace/SOCK_Connector.h"
#include "ace/Log_Msg.h"


TcpPublisher::TcpPublisher(const ACE_INET_Addr& addr)
  : subscriber_addr_(addr)
{
}


TcpPublisher::~TcpPublisher()
{
}


void
TcpPublisher::connect()
{
  ACE_SOCK_Connector connector;

  if (connector.connect(subscriber_, subscriber_addr_) == -1) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Publisher cannot connect to subscriber.\n"));
    throw TestException();
  }
}


void
TcpPublisher::disconnect()
{
  if (subscriber_.close() == -1 ) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Publisher cannot disconnect from the subscriber.\n"));
  }
}


void
TcpPublisher::send_bytes(unsigned num_bytes, const char* bytes)
{
  if (subscriber_.send_n(bytes, num_bytes) == -1) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Publisher cannot send bytes to subscriber.\n"));
    throw TestException();
  }
}
