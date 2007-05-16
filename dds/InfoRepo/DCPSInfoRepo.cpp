#include "DcpsInfo_pch.h"
#include "DCPSInfo_i.h"

#include "dds/DCPS/Service_Participant.h"

#include "dds/DCPS/transport/framework/EntryExit.h"

//If we need BIT support, pull in TCP so that static builds will have it.
#if !defined(DDS_HAS_MINIMUM_BIT)
#include "dds/DCPS/transport/simpleTCP/SimpleTcp.h"
#endif

#include "tao/ORB_Core.h"
#include "tao/IORTable/IORTable.h"

#include "ace/Get_Opt.h"
#include "ace/Arg_Shifter.h"
#include "ace/Service_Config.h"

#include <string>
#include <fstream>
#include <iostream>

class InfoRepo
{
public:
  struct InitError
  {
    InitError (const char* msg)
      : msg_(msg) {};
    std::string msg_;
  };

  InfoRepo (int argc, ACE_TCHAR *argv[]) throw (InitError);
  ~InfoRepo (void);
  bool run (void);

private:
  bool init (int argc, ACE_TCHAR *argv[]) throw (InitError);
  void usage (const ACE_TCHAR * cmd);
  void parse_args (int argc, ACE_TCHAR *argv[]);

  CORBA::ORB_var orb_;
  PortableServer::POA_var root_poa_;
  PortableServer::POA_var info_poa_;
  PortableServer::POAManager_var poa_manager_;

  auto_ptr<TAO_DDS_DCPSInfo_i> info_;

  ACE_TString ior_file_;
  ACE_TString domain_file_;
  ACE_TString listen_address_str_;
  int listen_address_given_;
  bool use_bits_;
  bool resurrect_;
};

InfoRepo::InfoRepo (int argc, ACE_TCHAR *argv[]) throw (InitError)
  : ior_file_ (ACE_TEXT("repo.ior"))
    , domain_file_ (ACE_TEXT("domain_ids"))
    , listen_address_given_ (0)
    , use_bits_ (true)
    , resurrect_ (true)
{
  listen_address_str_ = ACE_LOCALHOST;
  listen_address_str_ += ":2839";

  init (argc, argv);
}

InfoRepo::~InfoRepo (void)
{
  TheTransportFactory->release();
  TheServiceParticipant->shutdown ();

  orb_->destroy ();
}

bool
InfoRepo::run (void)
{
  orb_->run ();

  return true;
}

void
InfoRepo::usage (const ACE_TCHAR * cmd)
{
  ACE_DEBUG ((LM_INFO,
              ACE_TEXT ("Usage:\n")
              ACE_TEXT ("  %s\n")
              ACE_TEXT ("    -a <address> listening address for Built-In Topics\n")
              ACE_TEXT ("    -o <file> write ior to file\n")
              ACE_TEXT ("    -d <file> load domain ids from file\n")
              ACE_TEXT ("    -NOBITS disable the Built-In Topics\n")
              ACE_TEXT ("    -z turn on verbose Transport logging\n")
              ACE_TEXT ("    -r Resurrect from persistent file\n")
              ACE_TEXT ("    -?\n")
              ACE_TEXT ("\n"),
              cmd));
}

void
InfoRepo::parse_args (int argc,
                      ACE_TCHAR *argv[])
{
  ACE_Arg_Shifter arg_shifter(argc, argv);

  const char* current_arg = 0;

  while (arg_shifter.is_anything_left())
    {
      if ( (current_arg = arg_shifter.get_the_parameter("-a")) != 0)
        {
          listen_address_str_ = current_arg;
          listen_address_given_ = 1;
          arg_shifter.consume_arg();
        }
      else if ((current_arg = arg_shifter.get_the_parameter
                ("-r")) != 0)
        {
          int p = ACE_OS::atoi (current_arg);
          resurrect_ = true;

          if (p == 0) {
            resurrect_ = false;
          }
          arg_shifter.consume_arg ();
        }
      else if ((current_arg = arg_shifter.get_the_parameter("-d")) != 0)
        {
          domain_file_ = current_arg;
          arg_shifter.consume_arg ();
        }
      else if ((current_arg = arg_shifter.get_the_parameter("-o")) != 0)
        {
          ior_file_ = current_arg;
          arg_shifter.consume_arg ();
        }
      else if (arg_shifter.cur_arg_strncasecmp("-NOBITS") == 0)
        {
          use_bits_ = false;
          arg_shifter.consume_arg ();
        }
      else if (arg_shifter.cur_arg_strncasecmp("-z") == 0)
        {
          TURN_ON_VERBOSE_DEBUG;
          arg_shifter.consume_arg();
        }

      // The '-?' option
      else if (arg_shifter.cur_arg_strncasecmp("-?") == 0)
        {
          this->usage (argv[0]);
          ACE_OS::exit (0);

        }
      // Anything else we just skip

      else
        {
          arg_shifter.ignore_arg();
        }
    }
}

bool
InfoRepo::init (int argc, ACE_TCHAR *argv[]) throw (InitError)
{
  orb_ = CORBA::ORB_init (argc, argv, "");
  info_.reset(new TAO_DDS_DCPSInfo_i (orb_.in(), resurrect_));

  // ciju: Hard-code the 'RW' wait strategy directive.
  // Deadlocks have been observed to occur otherwise under stress conditions.
  ACE_Service_Config::process_directive
    (ACE_TEXT("static Client_Strategy_Factory \"-ORBWaitStrategy rw ")
     ACE_TEXT("-ORBTransportMuxStrategy exclusive -ORBConnectStrategy blocked\""));
  ACE_Service_Config::process_directive
    (ACE_TEXT("static Resource_Factory \"-ORBFlushingStrategy blocking\""));

  CORBA::Object_var obj =
    orb_->resolve_initial_references ("RootPOA");
  root_poa_ = PortableServer::POA::_narrow (obj.in ());

  poa_manager_ =
    root_poa_->the_POAManager ();

  // Use persistent and user id POA policies so the Info Repo's
  // object references are consistent.
  CORBA::PolicyList policies (2);
  policies.length (2);
  policies[0] =
    root_poa_->create_id_assignment_policy (PortableServer::USER_ID);
  policies[1] =
    root_poa_->create_lifespan_policy (PortableServer::PERSISTENT);
  info_poa_ = root_poa_->create_POA ("InfoRepo",
                                     poa_manager_.in (),
                                     policies);

  // Creation of the new POAs over, so destroy the Policy_ptr's.
  for (CORBA::ULong i = 0; i < policies.length (); ++i) {
    policies[i]->destroy ();
  }

  PortableServer::ObjectId_var oid =
    PortableServer::string_to_ObjectId ("InfoRepo");
  info_poa_->activate_object_with_id (oid.in (),
                                      info_.get());
  obj = info_poa_->id_to_reference(oid.in());
  TAO::DCPS::DCPSInfo_var info_repo
    = TAO::DCPS::DCPSInfo::_narrow (obj.in ());

  CORBA::String_var objref_str =
    orb_->object_to_string (info_repo.in ());

  TheServiceParticipant->set_ORB(orb_.in());
  TheServiceParticipant->set_repo_ior(objref_str.in());

  // Initialize the DomainParticipantFactory
  ::DDS::DomainParticipantFactory_var dpf
      = TheParticipantFactoryWithArgs(argc, argv);

  // We need parse the command line options for DCPSInfoRepo after parsing DCPS specific
  // command line options.

  // Check the non-ORB arguments.
  this->parse_args (argc, argv);

  // Activate the POA manager before initialize built-in-topics
  // so invocations can be processed.
  poa_manager_->activate ();

  if (use_bits_)
    {
      ACE_INET_Addr address (listen_address_str_.c_str());

      if (0 != info_->init_transport(listen_address_given_, address))
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("ERROR: Failed to initialize the transport!\n")),
                           false);
        }
    }

  // Load the domains _after_ initializing the participant factory and initializing
  // the transport
  if (0 >= info_->load_domains (domain_file_.c_str(), use_bits_))
    {
      //ACE_ERROR_RETURN((LM_ERROR, "ERROR: Failed to load any domains!\n"), -1);
    }

  CORBA::Object_var table_object =
    orb_->resolve_initial_references ("IORTable");

  IORTable::Table_var adapter =
    IORTable::Table::_narrow (table_object.in ());
  if (CORBA::is_nil (adapter.in ())) {
    ACE_ERROR ((LM_ERROR, "Nil IORTable\n"));
  }
  else {
    adapter->bind ("DCPSInfoRepo", objref_str.in ());
  }

  std::ofstream ior_stream (ior_file_.c_str());
  if (!ior_stream)
    {
      ACE_ERROR((LM_ERROR, "ERROR: Unable to open IOR file: %s\n"
                 , ior_file_.c_str()));
      throw InitError ("Unable to open IOR file.");
  }
  ior_stream << objref_str.in ();

  return true;
}


int
ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{

  try
    {
      InfoRepo infoRepo (argc, argv);

      infoRepo.run ();
    }
  catch (InfoRepo::InitError& ex)
    {
      std::cerr << "Unexpected initialization Error: "
                << ex.msg_ << std::endl;
      return -1;
    }
  catch (const CORBA::Exception& ex)
    {
      ex._tao_print_exception (
        "ERROR: ::DDS DCPS Info Repo caught exception");

      return -1;
    }

  return 0;
}
