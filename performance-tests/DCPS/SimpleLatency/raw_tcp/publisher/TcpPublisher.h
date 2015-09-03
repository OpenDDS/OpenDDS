#ifndef TCPPUBLISHER_H
#define TCPPUBLISHER_H

#include "ace/SOCK_Stream.h"
#include <ace/High_Res_Timer.h>
#include "ace/Synch.h"
#include "ace/Condition_T.h"
#include "ace/Acceptor.h"
#include "ace/SOCK_Acceptor.h"


class TcpPublisher
{
  public:

    TcpPublisher(const ACE_INET_Addr& sub_addr,
                 unsigned message_size);
    virtual ~TcpPublisher();

    void connect();
    void disconnect();

    void send_bytes(unsigned num_bytes, const char* bytes);

    void dump_stats ();

  private:

    ACE_INET_Addr      subscriber_addr_;
    ACE_SOCK_Stream    subscriber_;
    ACE_Message_Block  buffer_;
    int                pkt_count_;
};

#endif
