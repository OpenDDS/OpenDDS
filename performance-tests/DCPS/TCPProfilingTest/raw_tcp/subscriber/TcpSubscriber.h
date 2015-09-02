#ifndef TCPSUBSCRIBER_H
#define TCPSUBSCRIBER_H

#include "TestStats.h"
#include "ace/SOCK_Stream.h"
#include "ace/Svc_Handler.h"
#include "ace/Reactor.h"
#include "ace/Malloc_T.h"


class TcpSubscriber : public ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_NULL_SYNCH>
{
  public:

    typedef ACE_Dynamic_Cached_Allocator<ACE_SYNCH_NULL_MUTEX> TestAllocator;

    TcpSubscriber();
    virtual ~TcpSubscriber();

    static void initSubscriber(TestStats*     stats,
                               ACE_Reactor*   r,
                               TestAllocator* allocator,
                               unsigned       block_size);

    virtual int open(void*);
    virtual int close(u_long);

    virtual void destroy();


  protected:

    virtual int handle_input(ACE_HANDLE);
    virtual int handle_close(ACE_HANDLE, ACE_Reactor_Mask);


  private:

    unsigned           packet_count_;
    ACE_Message_Block* remainder_;

    static TestStats*     stats_;
    static ACE_Reactor*   r_;
    static TestAllocator* allocator_;
    static uintptr_t      block_size_;
};

#endif
