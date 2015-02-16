
#include "ace/Svc_Handler.h"
#include "ace/SOCK_Stream.h"
#include "ace/Select_Reactor.h"
#include "ace/Acceptor.h"
#include "ace/Connector.h"
#include "ace/SOCK_Acceptor.h"
#include "ace/SOCK_Connector.h"

#include <iostream>
#include <string>

class Pub_Handler;

int debug_level = 0;
std::string sub_addr_str; // = "localhost:1234";
std::string pub_addr_str; // = "localhost:1235";

class Sub_Handler : public ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_MT_SYNCH>
{
public:
  Sub_Handler (ACE_Thread_Manager *thr_mgr = 0)
    : ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_MT_SYNCH> (thr_mgr),
      pub_handler_(0)
  {
    if (debug_level) {
      ACE_DEBUG((LM_DEBUG, "stub(%P): Sub_Handler thread manager constructor, this=%@\n", this));
    }
  }
  Sub_Handler (Pub_Handler *pub_handler)
    : ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_MT_SYNCH> (0),
      pub_handler_(pub_handler)
  {
    if (debug_level) {
      ACE_DEBUG((LM_DEBUG, "stub(%P): Sub_Handler pub_handler constructor, this=%@\n", this));
    }
  }
  virtual int handle_input (ACE_HANDLE fd = ACE_INVALID_HANDLE);
  virtual int handle_close (ACE_HANDLE fd, ACE_Reactor_Mask = 0) {
    if (debug_level) {
      ACE_DEBUG((LM_DEBUG, "stub(%P): Sub_Handler handle_close(), this=%@, fd=0x%x\n", this, fd));
    }
    this->destroy ();
    return 0;
  }

private:
  Pub_Handler *pub_handler_;
};

class Pub_Handler : public ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_MT_SYNCH>
{
public:
  Pub_Handler (ACE_Thread_Manager *thr_mgr = 0)
    : ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_MT_SYNCH> (thr_mgr)
  {
    if (debug_level) {
      ACE_DEBUG((LM_DEBUG, "stub(%P): Pub_Handler constructor, this=%@\n", this));
    }

    ACE_INET_Addr sub_addr (sub_addr_str.c_str());
    ACE_Connector<Sub_Handler, ACE_SOCK_CONNECTOR> connector;
    sub_handler_ = new Sub_Handler(this);
    ACE_DEBUG((LM_DEBUG, "stub(%P): connecting on %C\n", sub_addr_str.c_str()));
    if (connector.connect (sub_handler_, sub_addr) < 0) {
      ACE_ERROR ((LM_ERROR, "(%t) %p\n", "connect"));
      return;
    }
    if (debug_level) {
      ACE_DEBUG((LM_DEBUG, "stub(%P): Connection succeeded\n"));
    }
    this->reactor (ACE_Reactor::instance ());
  }

  virtual ~Pub_Handler() {
    if (debug_level) {
      ACE_DEBUG((LM_DEBUG, "stub(%P): Pub_Handler destructor, this=%@\n", this));
    }
  }

  virtual int handle_input (ACE_HANDLE fd = ACE_INVALID_HANDLE) {
    if (debug_level) {
      ACE_DEBUG((LM_DEBUG, "stub(%P): Pub_Handler handle_input(), this=%@\n", this));
    }
    iovec iov;
    size_t result = this->peer ().recvv (&iov);
    if (result > 0) {
      if (debug_level) {
        ACE_DEBUG ((LM_DEBUG, "stub(%P): read %d bytes from publisher\n", result));
      }
      size_t sent = sub_handler_->peer().sendv_n(&iov, 1);
      if (debug_level) {
        ACE_DEBUG ((LM_DEBUG, "stub(%P): wrote %d bytes to subscriber\n", sent));
      }
      delete[] (char*) iov.iov_base;
    } else if (result <= 0) {
      ACE_DEBUG ((LM_DEBUG, "(%t) Pub_Handler: 0x%x peer closed (0x%x)\n", this, fd));
      return -1;
    }
    return 0;
  }
  virtual int handle_close (ACE_HANDLE fd, ACE_Reactor_Mask = 0) {
    ACE_DEBUG((LM_DEBUG, "stub(%P): Pub_Handler handle_close(), this=%@, fd: 0x%x\n", this, fd));
    this->destroy ();
    return 0;
  }

private:
  ACE_SOCK_Stream sub_stream_;
  Sub_Handler* sub_handler_;
};

int Sub_Handler::handle_input (ACE_HANDLE fd) {
  ACE_DEBUG((LM_DEBUG, "stub(%P): Sub_Handler handle_input(), this=%@\n", this));
  iovec iov;
  size_t result = this->peer ().recvv (&iov);
  if (result > 0) {
    if (debug_level) {
      ACE_DEBUG ((LM_DEBUG, "stub(%P): read %d bytes from subscriber\n", result));
    }
    size_t sent = pub_handler_->peer().sendv_n(&iov, 1);
    if (debug_level) {
      ACE_DEBUG ((LM_DEBUG, "stub(%P): wrote %d bytes to publisher\n", sent));
    }
    delete[] (char*) iov.iov_base;
  } else if (result <= 0) {
    ACE_DEBUG ((LM_DEBUG, "stub(%P): Sub_Handler: 0x%x peer closed (0x%x)\n", this, fd));
    return -1;
  }
  return 0;
}

void usage(const std::string& msg) {
  std::cerr << msg;
  std::cerr << "\n\nUsage:\n"
               "stub [-v] -s:<sub_addr> -p:<pub_addr> -stub_ready_file:<file_name>\n"
               "\n"
               "  -v             Verbose.  Enables debug output.\n"
               "  -p:<pub_addr>  Publisher address. Stub will accept connections\n"
               "                 on this address.\n"
               "  -s:<sub_addr>  Subscriber address. Stub will connect to this address\n"
               "                 whenever a connection is accepted on the pub_addr.\n"
               "  -stub_ready_file:<file_name>   File name stub will write to when ready\n"
               "                 to accept connections from publisher.\n"
               "\n"
               "  Once connections have been established, stub will pass through all\n"
               "  data between the publisher and subscriber addresses.\n" << std::endl;
  exit(1);
}

struct Shutdown_Event_Handler : ACE_Event_Handler
{
  int handle_signal(int, siginfo_t*, ucontext_t*)
  {
    ACE_Reactor::instance()->end_reactor_event_loop();
    return 0;
  }
};

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  std::string stub_ready_filename;

  int i = 1;
  while (i < argc) {
    std::string arg = ACE_TEXT_ALWAYS_CHAR(argv[i]);
    if (arg == "-v") {
      debug_level = 1;
    } else if (arg.substr(0, 3) == "-s:") {
      sub_addr_str = arg.substr(3);
    } else if (arg.substr(0, 3) == "-p:") {
      pub_addr_str = arg.substr(3);
    } else if (arg.substr(0, 17) == "-stub_ready_file:") {
      stub_ready_filename = arg.substr(17).c_str();
    } else {
      usage(std::string("Invalid argument: ") + arg);
    }
    i++;
  }
  if ((sub_addr_str == "") || (pub_addr_str == "")) {
    usage("Must specify publisher-side and subscriber-side addresses");
  }

  ACE_Select_Reactor reactor;
  typedef ACE_Strategy_Acceptor <Pub_Handler, ACE_SOCK_ACCEPTOR> Acceptor;
  Acceptor acceptor;
  ACE_INET_Addr pub_addr(pub_addr_str.c_str());

  ACE_DEBUG((LM_DEBUG, "stub(%P): accepting on %C\n", pub_addr_str.c_str()));
  if (acceptor.open (pub_addr) == -1) {
    ACE_ERROR_RETURN ((LM_ERROR, ACE_TEXT ("%p open")), 1);
  }

  // Indicate that the publisher is ready
  FILE* stub_ready = ACE_OS::fopen(stub_ready_filename.c_str(), ACE_TEXT("w"));
  if (stub_ready == 0) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Unable to create stub ready file\n")));
  }
  ACE_OS::fclose(stub_ready);

  Shutdown_Event_Handler shutdown_eh;
  ACE_Sig_Handler shutdown_handler;
  shutdown_handler.register_handler(SIGINT, &shutdown_eh);
#ifdef ACE_WIN32
  shutdown_handler.register_handler(SIGBREAK, &shutdown_eh);
#endif

  const int result = ACE_Reactor::instance()->run_reactor_event_loop();
  ACE_DEBUG((LM_DEBUG, "stub(%P): exiting %d\n", result));
  return result;
}
