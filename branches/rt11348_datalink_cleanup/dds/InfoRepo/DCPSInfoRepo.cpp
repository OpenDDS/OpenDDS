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

#include <orbsvcs/Shutdown_Utilities.h>

#ifdef ACE_AS_STATIC_LIBS
#include <tao/Version.h>
#if TAO_MAJOR_VERSION == 1 && TAO_MINOR_VERSION == 4
// no ImR_Client, and we are already including it statically since we are
// statically linking to TAO_PortableServer
#else
#include "tao/ImR_Client/ImR_Client.h"
#endif
#endif

#include "ace/Get_Opt.h"
#include "ace/Arg_Shifter.h"
#include "ace/Service_Config.h"
#include "ace/Argv_Type_Converter.h"

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
  void shutdown (void);

private:
  bool init (int argc, ACE_TCHAR *argv[]) throw (InitError);
  void usage (const ACE_TCHAR * cmd);
  void parse_args (int argc, ACE_TCHAR *argv[]);

  CORBA::ORB_var orb_;
  PortableServer::POA_var root_poa_;
  PortableServer::POA_var info_poa_;
  PortableServer::POAManager_var poa_manager_;

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
  listen_address_str_ += ACE_TEXT(":2839");

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
InfoRepo::shutdown (void)
{
  orb_->shutdown (0);
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

  const ACE_TCHAR* current_arg = 0;

  while (arg_shifter.is_anything_left())
    {
      if ( (current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-a"))) != 0)
        {
          listen_address_str_ = current_arg;
          listen_address_given_ = 1;
          arg_shifter.consume_arg();
        }
      else if ((current_arg = arg_shifter.get_the_parameter
                (ACE_TEXT("-r"))) != 0)
        {
          int p = ACE_OS::atoi (current_arg);
          resurrect_ = true;

          if (p == 0) {
            resurrect_ = false;
          }
          arg_shifter.consume_arg ();
        }
      else if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-d"))) != 0)
        {
          domain_file_ = current_arg;
          arg_shifter.consume_arg ();
        }
      else if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-o"))) != 0)
        {
          ior_file_ = current_arg;
          arg_shifter.consume_arg ();
        }
      else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-NOBITS")) == 0)
        {
          use_bits_ = false;
          arg_shifter.consume_arg ();
        }
      else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-z")) == 0)
        {
          TURN_ON_VERBOSE_DEBUG;
          arg_shifter.consume_arg();
        }

      // The '-?' option
      else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-?")) == 0)
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
  ACE_Argv_Type_Converter cvt (argc, argv);

  orb_ = CORBA::ORB_init (cvt.get_argc(), cvt.get_ASCII_argv(), "");
  PortableServer::ServantBase_var info(new TAO_DDS_DCPSInfo_i (orb_.in(), resurrect_));
  TAO_DDS_DCPSInfo_i* info_servant = dynamic_cast<TAO_DDS_DCPSInfo_i*>(info.in());

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
                                      info.in());
  obj = info_poa_->id_to_reference(oid.in());
  // the object is created locally, so it is safe to do an 
  // _unchecked_narrow, this was needed to prevent an exception
  // when dealing with ImR-ified objects
  OpenDDS::DCPS::DCPSInfo_var info_repo
    = OpenDDS::DCPS::DCPSInfo::_unchecked_narrow (obj.in ());

  CORBA::String_var objref_str =
    orb_->object_to_string (info_repo.in ());

  TheServiceParticipant->set_ORB(orb_.in());

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

      if (0 != info_servant->init_transport(listen_address_given_, address))
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("ERROR: Failed to initialize the transport!\n")),
                           false);
        }
    }
  else
    {
      TheServiceParticipant->set_BIT(false);
    }

  // This needs to be done after initialization since we create the reference
  // to ourselves in the service here.
  TheServiceParticipant->set_repo_ior(
    ACE_TEXT_CHAR_TO_TCHAR(objref_str.in()),
    OpenDDS::DCPS::Service_Participant::DEFAULT_REPO
  );

  // Load the domains _after_ initializing the participant factory and initializing
  // the transport
  if (0 >= info_servant->load_domains (domain_file_.c_str(), use_bits_))
    {
      //ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("ERROR: Failed to load any domains!\n")), -1);
    }

  CORBA::Object_var table_object =
    orb_->resolve_initial_references ("IORTable");

  IORTable::Table_var adapter =
    IORTable::Table::_narrow (table_object.in ());
  if (CORBA::is_nil (adapter.in ())) {
    ACE_ERROR ((LM_ERROR, ACE_TEXT("Nil IORTable\n")));
  }
  else {
    adapter->bind ("DCPSInfoRepo", objref_str.in ());
  }

  FILE *output_file= ACE_OS::fopen (ior_file_.c_str (), ACE_TEXT("w"));
  if (output_file == 0) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Unable to open IOR file: %s\n")
                 , ior_file_.c_str()));
      throw InitError ("Unable to open IOR file.");
  }

  ACE_OS::fprintf (output_file, "%s", objref_str.in ());
  ACE_OS::fclose (output_file);

  return true;
}

class InfoRepo_Shutdown : public Shutdown_Functor
{
public:
  InfoRepo_Shutdown(InfoRepo& ir);

  void operator() (int which_signal);
private:
  InfoRepo& ir_;
};

InfoRepo_Shutdown::InfoRepo_Shutdown (InfoRepo &ir)
  : ir_(ir)
{
}

void
InfoRepo_Shutdown::operator() (int which_signal)
{
  ACE_DEBUG ((LM_DEBUG,
              "InfoRepo_Shutdown: shutting down on signal %d\n",
              which_signal));
  ir_.shutdown ();
}

int
ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{

  try
    {
      InfoRepo infoRepo (argc, argv);

      InfoRepo_Shutdown ir_shutdown (infoRepo);
      Service_Shutdown service_shutdown(ir_shutdown);

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
