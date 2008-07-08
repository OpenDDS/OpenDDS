#ifndef TCPPUBLISHER_H
#define TCPPUBLISHER_H

#include "ace/SOCK_Stream.h"


class TcpPublisher
{
  public:

    TcpPublisher(const ACE_INET_Addr& addr);
    virtual ~TcpPublisher();

    void connect();
    void disconnect();

    void send_bytes(unsigned num_bytes, const char* bytes);


  private:

    ACE_INET_Addr   subscriber_addr_;
    ACE_SOCK_Stream subscriber_;
};

#endif
