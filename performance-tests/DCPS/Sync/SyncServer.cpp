#include "SyncServer_i.h"

#include "ace/Get_Opt.h"
#include "ace/String_Base.h"

#include <memory>
#include <string>
#include <iostream>
#include <fstream>

class SyncServer
{
public:
  typedef std::string InitError;

  SyncServer (int argc, ACE_TCHAR *argv[]) throw (InitError);

  bool run ();

private:
  bool parse_args (int argc, ACE_TCHAR *argv[]);

  std::auto_ptr<SyncServer_i> sync_server_;

  size_t pub_count_;
  size_t sub_count_;
};

bool
SyncServer::parse_args (int argc, ACE_TCHAR *argv[])
{
  ACE_Get_Opt get_opts (argc, argv, ACE_TEXT("p:s:"));
  int c;
  std::string usage =
    " -p <publisher count>\n"
    " -s <subscriber count>\n";

  while ((c = get_opts ()) != -1)
  {
    switch (c)
      {
      case 'p':
        pub_count_ = ACE_OS::atoi (get_opts.opt_arg ());
        break;
      case 's':
        sub_count_ = ACE_OS::atoi (get_opts.opt_arg ());
        break;
      case '?':
      default:
        ACE_ERROR_RETURN ((LM_ERROR,
                           "usage> %s\n",
                           usage.c_str()),
                          false);
      }
  }


  return true;
}

SyncServer::SyncServer (int argc, ACE_TCHAR* argv[])
  throw (SyncServer::InitError)
  : pub_count_ (1), sub_count_ (1)
{
  try
    {
      this->parse_args (argc, argv);

      sync_server_.reset (new SyncServer_i (pub_count_, sub_count_
                                            , CORBA::ORB::_nil()));
    }
  catch (SyncServer_i::InitError& e) {
    throw InitError (e);
  }
  catch ( CORBA::Exception& ex) {
    throw InitError (ex._info().c_str());
  }
}

bool
SyncServer::run (void)
{
  sync_server_->wait_session ();
  return true;
}

int
ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{

  try
    {
      SyncServer sync_server (argc, argv);

      sync_server.run ();
    }
  catch (SyncServer::InitError& ex)
    {
      std::string& msg = reinterpret_cast<std::string&>(ex);

      std::cerr << "Initialization Error: "
                << msg.c_str() << std::endl;
      return -1;
    }
  catch (const CORBA::Exception& ex)
    {
      ex._tao_print_exception (
        "ERROR: SyncServer caught exception");

      return -1;
    }

  return 0;
}
