/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DcpsInfo_pch.h"
#include "tao/ORB_Core.h"
#include "DCPSInfo_i.h"
#include "DCPSInfoRepoServ.h"
#include "FederatorConfig.h"
#include "FederatorManagerImpl.h"
#include "ShutdownInterface.h"
#include "PersistenceUpdater.h"
#include "UpdateManager.h"

#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/InfoRepoDiscovery/InfoRepoDiscovery.h"

//If we need BIT support, pull in TCP so that static builds will have it.
#if !defined(DDS_HAS_MINIMUM_BIT)
#include "dds/DCPS/transport/tcp/Tcp.h"
#endif

#include "tao/IORTable/IORTable.h"
#include "tao/BiDir_GIOP/BiDirGIOP.h"

#include <orbsvcs/Shutdown_Utilities.h>

#ifdef ACE_AS_STATIC_LIBS
#include "tao/ImR_Client/ImR_Client.h"
#endif

#include "ace/Get_Opt.h"
#include "ace/Arg_Shifter.h"
#include "ace/Service_Config.h"
#include "ace/Argv_Type_Converter.h"

#include <string>
#include <sstream>

InfoRepo::InfoRepo(int argc, ACE_TCHAR *argv[])
: ior_file_(ACE_TEXT("repo.ior"))
, listen_address_given_(0)
#ifdef DDS_HAS_MINIMUM_BIT
, use_bits_(false)
#else
, use_bits_(true)
#endif
, resurrect_(true)
, finalized_(false)
, servant_finalized_(false)
, federator_(this->federatorConfig_)
, federatorConfig_(argc, argv)
, lock_()
, cond_(lock_)
, shutdown_complete_(false)
, dispatch_cleanup_delay_(30,0)
{
  try {
    this->init();
  } catch (...) {
    this->finalize();
    throw;
  }
}

InfoRepo::~InfoRepo()
{
  this->finalize();
}

void
InfoRepo::run()
{
  this->shutdown_complete_ = false;
  this->orb_->run();
  this->finalize();
  ACE_GUARD(ACE_Thread_Mutex, g, this->lock_);
  this->shutdown_complete_ = true;
  this->cond_.signal();
}

void
InfoRepo::finalize()
{
  if (this->finalized_) {
    return;
  }

  if (!this->servant_finalized_) {
    // reached if the ImR caused the ORB to shut down,
    // which bypasses InfoRepo::handle_exception()
    this->info_servant_->finalize();
    this->federator_.finalize();
    TheServiceParticipant->shutdown();
    this->servant_finalized_ = true;
  }

  if (!CORBA::is_nil(this->orb_)) {
    try {
      this->orb_->destroy();
    }
    catch (const CORBA::Exception&) {
      //finalizing anyway, not an issue.
    }
  }

  this->finalized_ = true;
}

int
InfoRepo::handle_exception(ACE_HANDLE /* fd */)
{
  // these should occur before ORB::shutdown() since they use the ORB/reactor
  this->info_servant_->finalize();
  this->federator_.finalize();
  TheServiceParticipant->shutdown();
  this->servant_finalized_ = true;

  this->orb_->shutdown(true);
  return 0;
}

void
InfoRepo::shutdown()
{
  this->orb_->orb_core()->reactor()->notify(this);
  // reactor will invoke our InfoRepo::handle_exception()
}

void
InfoRepo::sync_shutdown()
{
  this->shutdown();
  ACE_GUARD(ACE_Thread_Mutex, g, this->lock_);

  while (!this->shutdown_complete_) {
    this->cond_.wait();
  }
}

void
InfoRepo::usage(const ACE_TCHAR* cmd)
{
  // NOTE: The federation arguments are parsed early by the
  //       FederationConfig object.
  ACE_DEBUG((LM_INFO,
             ACE_TEXT("Usage:\n")
             ACE_TEXT("  %s\n")
             ACE_TEXT("    -a <address> listening address for Built-In Topics\n")
             ACE_TEXT("    -o <file> write ior to file\n")
             ACE_TEXT("    -NOBITS disable the Built-In Topics\n")
             ACE_TEXT("    -z turn on verbose Transport logging\n")
             ACE_TEXT("    -r Resurrect from persistent file\n")
             ACE_TEXT("    -FederatorConfig <file> configure federation from <file>\n")
             ACE_TEXT("    -FederationId <number> value for this repository\n")
             ACE_TEXT("    -FederateWith <ior> federate initially with object at <ior>\n")
             ACE_TEXT("    -ReassociateDelay <msec> delay between reassociations\n")
             ACE_TEXT("    -DispatchingCheckDelay <sec> delay between checks for cleaning up dispatching connections.\n")
             ACE_TEXT("    -?\n")
             ACE_TEXT("\n"),
             cmd));
}

void
InfoRepo::parse_args(int argc, ACE_TCHAR *argv[])
{
  ACE_Arg_Shifter arg_shifter(argc, argv);

  const ACE_TCHAR* current_arg = 0;

  while (arg_shifter.is_anything_left()) {
    if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-a"))) != 0) {
      this->listen_address_str_ = ACE_TEXT_ALWAYS_CHAR(current_arg);
      this->listen_address_given_ = 1;
      arg_shifter.consume_arg();
    // Must check for -ReassociateDelay before -r
    } else if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-ReassociateDelay"))) != 0) {
      long msec = ACE_OS::atoi(current_arg);
      this->reassociate_delay_.msec(msec);

      arg_shifter.consume_arg();
    } else if ((current_arg = arg_shifter.get_the_parameter
                              (ACE_TEXT("-r"))) != 0) {
      int p = ACE_OS::atoi(current_arg);
      this->resurrect_ = true;

      if (p == 0) {
        this->resurrect_ = false;
      }

      arg_shifter.consume_arg();

    } else if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-o"))) != 0) {
      this->ior_file_ = current_arg;
      arg_shifter.consume_arg();

    } else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-NOBITS")) == 0) {
      this->use_bits_ = false;
      arg_shifter.consume_arg();

    } else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-z")) == 0) {
      TURN_ON_VERBOSE_DEBUG;
      arg_shifter.consume_arg();

    } else if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-DispatchingCheckDelay"))) != 0) {
      long sec = ACE_OS::atoi(current_arg);
      this->dispatch_cleanup_delay_.sec(sec);
      arg_shifter.consume_arg();

    }

    // The '-?' option
    else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-?")) == 0) {
      this->usage(argv[0]);
      throw InitError("Usage");
    }

    // Anything else we just skip

    else {
      arg_shifter.ignore_arg();
    }
  }
}

void
InfoRepo::init()
{
  ACE_ARGV args;
  args.add(federatorConfig_.argv(), true /*quote arg*/);

  bool use_bidir = true;

  for (int i = 0; i < args.argc() - 1; ++i) {
    if (0 == ACE_OS::strcmp(args[i], ACE_TEXT("-DCPSBidirGIOP"))) {
      use_bidir = ACE_OS::atoi(args[i + 1]);
      break;
    }
  }

  if (use_bidir) {
    const ACE_TCHAR* config[] = {
      ACE_TEXT("-ORBSvcConfDirective"),
      ACE_TEXT("static Client_Strategy_Factory \"-ORBWaitStrategy rw ")
        ACE_TEXT("-ORBTransportMuxStrategy exclusive -ORBConnectStrategy blocked ")
        ACE_TEXT("-ORBConnectionHandlerCleanup 1\""),
      ACE_TEXT("-ORBSvcConfDirective"),
      ACE_TEXT("static Resource_Factory \"-ORBFlushingStrategy blocking\""),
      0
    };
    args.add((ACE_TCHAR**)config, true /*quote arg*/);
  }

  int argc = args.argc();
  orb_ = CORBA::ORB_init(argc, args.argv());

  this->info_servant_ =
    new TAO_DDS_DCPSInfo_i(this->orb_, this->resurrect_, this,
                           this->federatorConfig_.federationId());

  // Install the DCPSInfo_i into the Federator::Manager.
  this->federator_.info() = this->info_servant_.in();

  CORBA::Object_var obj =
    this->orb_->resolve_initial_references("RootPOA");
  PortableServer::POA_var root_poa = PortableServer::POA::_narrow(obj);

  PortableServer::POAManager_var poa_manager = root_poa->the_POAManager();

  // Use persistent and user id POA policies so the Info Repo's
  // object references are consistent.
  CORBA::PolicyList policies(2 + use_bidir);
  policies.length(2 + use_bidir);
  policies[0] = root_poa->create_id_assignment_policy(PortableServer::USER_ID);
  policies[1] = root_poa->create_lifespan_policy(PortableServer::PERSISTENT);
  if (use_bidir) {
    CORBA::Any policy;
    policy <<= BiDirPolicy::BOTH;
    policies[2] =
      orb_->create_policy(BiDirPolicy::BIDIRECTIONAL_POLICY_TYPE, policy);
  }
  PortableServer::POA_var info_poa = root_poa->create_POA("InfoRepo",
                                                          poa_manager,
                                                          policies);

  // Creation of the new POAs over, so destroy the Policy_ptr's.
  for (CORBA::ULong i = 0; i < policies.length(); ++i) {
    policies[i]->destroy();
  }

  PortableServer::ObjectId_var oid =
    PortableServer::string_to_ObjectId("InfoRepo");
  info_poa->activate_object_with_id(oid, this->info_servant_.in());
  obj = info_poa->id_to_reference(oid);
  // the object is created locally, so it is safe to do an
  // _unchecked_narrow, this was needed to prevent an exception
  // when dealing with ImR-ified objects
  OpenDDS::DCPS::DCPSInfo_var info_repo =
    OpenDDS::DCPS::DCPSInfo::_unchecked_narrow(obj);

  CORBA::String_var objref_str =
    orb_->object_to_string(info_repo);

  // RTPS discovery is the default, change it to InfoRepo
  TheServiceParticipant->set_default_discovery(OpenDDS::DCPS::Discovery::DEFAULT_REPO);

  // Initialize the DomainParticipantFactory
  DDS::DomainParticipantFactory_var dpf =
    TheParticipantFactoryWithArgs(argc, args.argv());

  // We need parse the command line options for DCPSInfoRepo after parsing DCPS specific
  // command line options.

  // Check the non-ORB arguments.
  this->parse_args(argc, args.argv());

  // Activate the POA manager before initialize built-in-topics
  // so invocations can be processed.
  poa_manager->activate();

  if (this->use_bits_) {
    if (this->info_servant_->init_transport(this->listen_address_given_,
        this->listen_address_str_.c_str())
        != 0 /* init_transport returns 0 for success */) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: DCPSInfoRepo::init: ")
                 ACE_TEXT("Unable to initialize transport.\n")));
      throw InitError("Unable to initialize transport.");
    }

  } else {
    TheServiceParticipant->set_BIT(false);
  }

  // This needs to be done after initialization since we create the reference
  // to ourselves in the service here.
  OpenDDS::DCPS::Service_Participant* serv_part = TheServiceParticipant;
  serv_part->set_repo_ior(objref_str, OpenDDS::DCPS::Discovery::DEFAULT_REPO);

  OpenDDS::DCPS::Discovery_rch disc = serv_part->get_discovery(0 /*domainId*/);
  OpenDDS::DCPS::InfoRepoDiscovery_rch ird =
    OpenDDS::DCPS::static_rchandle_cast<OpenDDS::DCPS::InfoRepoDiscovery>(disc);
  if (!ird->set_ORB(orb_)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: DCPSInfoRepo::init: ")
               ACE_TEXT("Unable to set the ORB in InfoRepoDiscovery.\n")));
    throw InitError("Unable to set the ORB in InfoRepoDiscovery.");
  }

  // Initialize persistence _after_ initializing the participant factory
  // and initializing the transport.
  if (!this->info_servant_->init_persistence()) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: DCPSInfoRepo::init: ")
               ACE_TEXT("Unable to initialize persistence.\n")));
    throw InitError("Unable to initialize persistence.");
  }

  // Initialize reassociation.
  if (this->reassociate_delay_ != ACE_Time_Value::zero &&
     !this->info_servant_->init_reassociation(this->reassociate_delay_)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: DCPSInfoRepo::init: ")
               ACE_TEXT("Unable to initialize reassociation.\n")));
    throw InitError("Unable to initialize reassociation.");
  }

  // Initialize dispatch checking
  if (this->dispatch_cleanup_delay_ != ACE_Time_Value::zero &&
     !this->info_servant_->init_dispatchChecking(this->dispatch_cleanup_delay_)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: DCPSInfoRepo::init: ")
               ACE_TEXT("Unable to initialize Dispatch checking.\n")));
    throw InitError("Unable to initialize dispatch checking.");
  }

  // Fire up the federator.
  OpenDDS::Federator::Manager_var federator;
  CORBA::String_var               federator_ior;

  if (federator_.id().overridden()) {
    oid = PortableServer::string_to_ObjectId("Federator");
    info_poa->activate_object_with_id(oid, &federator_);
    obj = info_poa->id_to_reference(oid);
    federator = OpenDDS::Federator::Manager::_narrow(obj);

    federator_ior = orb_->object_to_string(federator);

    // Add a local repository reference that can be returned via a
    // remote call to a peer.
    this->federator_.localRepo(info_repo);

    // It should be safe to initialize the federation mechanism at this
    // point.  What we really needed to wait for is the initialization of
    // the service components - like the DomainParticipantFactory and the
    // repository bindings.
    // N.B. This is done *before* being added to the IOR table to avoid any
    //      races with an eager client.
    this->federator_.orb(this->orb_);

    //
    // Add the federator to the info_servant update manager as an
    // additional updater interface to be called.
    // N.B. This needs to be done *after* the call to load_domains()
    //      since that is where the update manager is initialized in the
    //      info startup sequencing.
    this->info_servant_->add(&this->federator_);
  }

  // Grab the IOR table.
  CORBA::Object_var table_object =
    this->orb_->resolve_initial_references("IORTable");

  IORTable::Table_var adapter = IORTable::Table::_narrow(table_object);

  if (CORBA::is_nil(adapter)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("Nil IORTable\n")));

  } else {
    adapter->bind(OpenDDS::Federator::REPOSITORY_IORTABLE_KEY, objref_str);

    if (this->federator_.id().overridden()) {
      // Bind to '/Federator'
      adapter->bind(OpenDDS::Federator::FEDERATOR_IORTABLE_KEY, federator_ior);

      // Bind to '/Federator/1382379631'
      std::stringstream buffer(OpenDDS::Federator::FEDERATOR_IORTABLE_KEY);
      buffer << "/" << std::dec << this->federatorConfig_.federationDomain();
      adapter->bind(buffer.str().c_str(), federator_ior);
    }
  }

  FILE* output_file = ACE_OS::fopen(this->ior_file_.c_str(), ACE_TEXT("w"));

  if (output_file == 0) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Unable to open IOR file: %s\n"),
               ior_file_.c_str()));
    throw InitError("Unable to open IOR file.");
  }

  ACE_OS::fprintf(output_file, "%s", objref_str.in());
  ACE_OS::fclose(output_file);

  // Initial federation join if specified on command line.
  if (this->federator_.id().overridden()
       && !this->federatorConfig_.federateIor().empty()) {
    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) INFO: DCPSInfoRepo::init() - ")
                 ACE_TEXT("joining federation with repository %s\n"),
                 this->federatorConfig_.federateIor().c_str()));
    }

    obj = this->orb_->string_to_object(
          this->federatorConfig_.federateIor().c_str());

    if (CORBA::is_nil(obj)) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: could not resolve %s for initial federation.\n"),
                 this->federatorConfig_.federateIor().c_str()));
      throw InitError("Unable to resolve IOR for initial federation.");
    }

    OpenDDS::Federator::Manager_var peer =
      OpenDDS::Federator::Manager::_narrow(obj);

    if (CORBA::is_nil(peer)) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: could not narrow %s.\n"),
                 this->federatorConfig_.federateIor().c_str()));
      throw InitError("Unable to narrow peer for initial federation.");
    }

    // Actually join.
    peer = peer->join_federation(federator,
                          this->federatorConfig_.federationDomain());
  }
}

InfoRepo_Shutdown::InfoRepo_Shutdown(InfoRepo &ir)
: ir_(ir)
{
}

void
InfoRepo_Shutdown::operator()(int which_signal)
{
  ACE_DEBUG((LM_DEBUG,
             "InfoRepo_Shutdown: shutting down on signal %d\n",
             which_signal));
  this->ir_.shutdown();
}
