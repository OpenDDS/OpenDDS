#include "SubDriver.h"
#include "TestException.h"

// Add the TransportImpl.h before TransportImpl_rch.h is included to
// resolve the build problem that the class is not defined when
// RcHandle<T> template is instantiated.
#include "dds/DCPS/transport/framework/TransportImpl.h"
#include "dds/DCPS/transport/tcp/TcpInst.h"

#include "dds/DCPS/transport/framework/TransportRegistry.h"
#include "dds/DCPS/transport/framework/NetworkAddress.h"
#include "dds/DCPS/transport/framework/EntryExit.h"

#include "dds/DCPS/AssociationData.h"
#include "dds/DCPS/RepoIdBuilder.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DdsDcpsInfoUtilsC.h"

#include <ace/Arg_Shifter.h>
#include <ace/OS_NS_sys_stat.h>
#include <ace/OS_NS_unistd.h>

#include <fstream>
#include <cstring>

SubDriver::SubDriver()
  : pub_id_(OpenDDS::DCPS::GuidBuilder::create())
  , sub_id_(OpenDDS::DCPS::GuidBuilder::create())
  , reader_(sub_id_)
  , num_msgs_(1)
  , shmem_(false)
{
  DBG_ENTRY("SubDriver","SubDriver");
}


SubDriver::~SubDriver()
{
  DBG_ENTRY("SubDriver","~SubDriver");
}


void
SubDriver::run(int& argc, ACE_TCHAR* argv[])
{
  DBG_ENTRY_LVL("SubDriver","run",6);

  DDS::DomainParticipantFactory_var dpf =
    TheParticipantFactoryWithArgs(argc, argv);

  parse_args(argc, argv);
  init();
  run();
}


void
SubDriver::parse_args(int& argc, ACE_TCHAR* argv[])
{
  DBG_ENTRY("SubDriver","parse_args");

  // Command-line arguments:
  //
  // -p <pub_id:pub_host:pub_port>
  // -s <sub_id:sub_port>
  // -n <num_messages>
  // -m                use shared memory
  //
  ACE_Arg_Shifter arg_shifter(argc, argv);

  bool got_n = false;
  bool got_p = false;
  bool got_s = false;

  const ACE_TCHAR* current_arg = 0;

  while (arg_shifter.is_anything_left())
  {
    // The '-p' option
    if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-p")))) {
      if (got_p) {
        ACE_ERROR((LM_ERROR,
                   "(%P|%t) Only one -p allowed on command-line.\n"));
        throw TestException();
      }

      int result = parse_pub_arg(current_arg);
      arg_shifter.consume_arg();

      if (result != 0) {
        ACE_ERROR((LM_ERROR,
                   "(%P|%t) Failed to parse -p command-line arg.\n"));
        throw TestException();
      }

      got_p = true;
    }
    // A '-s' option
    else if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-s")))) {
      if (got_s) {
        ACE_ERROR((LM_ERROR,
                   "(%P|%t) Only one -s allowed on command-line.\n"));
        throw TestException();
      }

      int result = parse_sub_arg(current_arg);
      arg_shifter.consume_arg();

      if (result != 0) {
        ACE_ERROR((LM_ERROR,
                   "(%P|%t) Failed to parse -s command-line arg.\n"));
        throw TestException();
      }

      got_s = true;
    }
    // The '-n' option
    else if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-n")))) {
      if (got_n) {
        ACE_ERROR((LM_ERROR,
                   "(%P|%t) Only one -n allowed on command-line.\n"));
        throw TestException();
      }

      int value = ACE_OS::atoi(current_arg);
      arg_shifter.consume_arg();

      if (value <= 0) {
        ACE_ERROR((LM_ERROR,
                   "(%P|%t) Value following -n option must be > 0.\n"));
        throw TestException();
      }

      this->num_msgs_ = value;

      got_n = true;
    }
    else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-m")) == 0) {
      this->shmem_ = true;
      arg_shifter.consume_arg();
    }
    // The '-?' option
    else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-?")) == 0) {
      ACE_DEBUG((LM_DEBUG,
                 "usage: %s "
                 "-p pub_id:pub_host:pub_port -s sub_id:sub_host:sub_port\n",
                 argv[0]));

      arg_shifter.consume_arg();
      throw TestException();
    }
    // Anything else we just skip
    else {
      arg_shifter.ignore_arg();
    }
  }

  // Make sure we got the required arguments:
  if (!got_p) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) -p command-line option not specified (required).\n"));
    throw TestException();
  }

  if (!got_s) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) -s command-line option not specified (required).\n"));
    throw TestException();
  }
}


void
SubDriver::init()
{
  DBG_ENTRY("SubDriver","init");

  OpenDDS::DCPS::TransportInst_rch inst;

  if (shmem_) {
    VDBG((LM_DEBUG, "(%P|%t) DBG:   Create a new ShmemInst object.\n"));

    inst = TheTransportRegistry->create_inst("shmem1", "shmem");

  } else {

    VDBG((LM_DEBUG, "(%P|%t) DBG:   Create a new TcpInst object.\n"));

    inst = TheTransportRegistry->create_inst("tcp1", "tcp");

    OpenDDS::DCPS::TcpInst_rch tcp_inst =
      OpenDDS::DCPS::dynamic_rchandle_cast<OpenDDS::DCPS::TcpInst>(inst);

    VDBG((LM_DEBUG, "(%P|%t) DBG:   "
               "Set the tcp_inst->local_address_ to our (local) sub_addr_.\n"));

    tcp_inst->local_address(ACE_TEXT_ALWAYS_CHAR(this->sub_addr_str_.c_str()));
  }

  OpenDDS::DCPS::TransportConfig_rch cfg =
    TheTransportRegistry->create_config("cfg");
  cfg->instances_.push_back(inst);

  TheTransportRegistry->global_config(cfg);

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "The Transport has been successfully configured.\n"));
}


void
SubDriver::run()
{
  DBG_ENTRY_LVL("SubDriver", "run", 6);

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Initialize our SimpleSubscriber object.\n"));

  this->reader_.enable_transport(false /*reliable*/, false /*durable*/);

  // Write a file so that test script knows we're ready
  FILE * file = ACE_OS::fopen ("subready.txt", ACE_TEXT("w"));
  ACE_OS::fprintf (file, "Ready\n");
  ACE_OS::fclose (file);

  VDBG((LM_DEBUG, "(%P|%t) DBG:   Create the 'publications'.\n"));

  // Set up the publication.
  OpenDDS::DCPS::AssociationData publication;
  publication.remote_id_ = this->pub_id_;
  publication.remote_reliable_ = true;
  publication.remote_data_.length(1);

  if (shmem_) {
    publication.remote_data_[0].transport_type = "shmem";

    std::ofstream ofs("sub-pid.txt");
    ofs << ACE_OS::getpid() << std::endl;
    ofs.close();

    for (ACE_stat filestat; -1 == ACE_OS::stat("pub-pid.txt", &filestat) || filestat.st_size == 0;
         ACE_OS::sleep(1)) {/*empty loop*/}

    std::ifstream ifs("pub-pid.txt");
    std::string pid;
    getline(ifs, pid);
    std::string str = OpenDDS::DCPS::get_fully_qualified_hostname() +
                      '\0' + "OpenDDS-" + pid + "-shmem1";

    publication.remote_data_[0].data.length(static_cast<CORBA::ULong>(str.size()));
    std::memcpy(publication.remote_data_[0].data.get_buffer(),
                str.c_str(), str.size());

  } else { // tcp
    publication.remote_data_[0].transport_type = "tcp";

    OpenDDS::DCPS::NetworkAddress network_order_address(
      ACE_TEXT_ALWAYS_CHAR(this->pub_addr_str_.c_str()));

    ACE_OutputCDR cdr;
    cdr << network_order_address;
    CORBA::ULong len = static_cast<CORBA::ULong>(cdr.total_length());

    publication.remote_data_[0].data =
      OpenDDS::DCPS::TransportBLOB(len, len, (CORBA::Octet*)(cdr.buffer()));
  }

  this->reader_.init(publication, this->num_msgs_);

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Ask the SimpleSubscriber object if it has received what, "
             "it expected.  If not, sleep for 1 second, and ask again.\n"));

  // Wait until we receive our expected message from the remote
  // publisher.  For this test, we should wait until we receive the
  // "Hello World!" message that we expect.  Then this program
  // can just shutdown.
  while (this->reader_.received_test_message() == 0) {
    ACE_OS::sleep(1);
  }

  this->reader_.print_time();

  if (shmem_) {
    ACE_OS::unlink("sub-pid.txt");
  }

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "The SimpleSubscriber object has received what it expected.  "
             "Release TheTransportFactory - causing all TransportImpl "
             "objects to be shutdown().\n"));

  this->reader_.disassociate(this->pub_id_);

  TheServiceParticipant->shutdown();

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "TheTransportFactory has finished releasing.\n"));
}


int
SubDriver::parse_pub_arg(const ACE_TString& arg)
{
  DBG_ENTRY("SubDriver","parse_pub_arg");

  size_t pos;

  // Find the first ':' character, and make sure it is in a legal spot.
  if ((pos = std::find(arg.c_str(), arg.c_str() + arg.length(), ACE_TEXT(':')) - arg.c_str()) == arg.length()) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Bad -p command-line value (%s). Missing ':' char.\n",
               arg.c_str()));
    return -1;
  }

  if (pos == 0) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Bad -p command-line value (%s). "
               "':' char cannot be first char.\n",
               arg.c_str()));
    return -1;
  }

  if (pos == (arg.length() - 1)) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Bad -p command-line value  (%s) - "
               "':' char cannot be last char.\n",
               arg.c_str()));
    return -1;
  }

  // Parse the pub_id from left of ':' char, and remainder to right of ':'.
  ACE_TString pub_id_str(arg.c_str(), pos);
  this->pub_addr_str_ = arg.c_str() + pos + 1;

  // RepoIds are conventionally created and managed by the DCPSInfoRepo. Those
  // generated here are for the sole purpose of verifying internal behavior.
  OpenDDS::DCPS::RepoIdBuilder builder(pub_id_);

  builder.participantId(1);
  builder.entityKey(ACE_OS::atoi(pub_id_str.c_str()));
  builder.entityKind(OpenDDS::DCPS::ENTITYKIND_USER_WRITER_WITH_KEY);

  // Find the (only) ':' char in the remainder, and make sure it is in
  // a legal spot.
  if ((pos = std::find(pub_addr_str_.c_str(), pub_addr_str_.c_str() + pub_addr_str_.length(), ACE_TEXT(':')) - pub_addr_str_.c_str()) == pub_addr_str_.length()) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Bad -p command-line value (%s). "
               "Missing second ':' char.\n",
               arg.c_str()));
    return -1;
  }

  if (pos == 0) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Bad -p command-line value (%s) - "
               "Second ':' char immediately follows first ':' char.\n",
               arg.c_str()));
    return -1;
  }

  if (pos == (arg.length() - 1)) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Bad -p command-line value (%s) - "
               "Second ':' char cannot be last char.\n",
               arg.c_str()));
    return -1;
  }

  // Use the remainder as the "stringified" ACE_INET_Addr.
  this->pub_addr_ = ACE_INET_Addr(this->pub_addr_str_.c_str());

  return 0;
}


int
SubDriver::parse_sub_arg(const ACE_TString& arg)
{
  DBG_ENTRY("SubDriver","parse_sub_arg");

  size_t pos;

  // Find the first ':' character, and make sure it is in a legal spot.
  if ((pos = std::find(arg.c_str(), arg.c_str() + arg.length(), ACE_TEXT(':')) - arg.c_str()) == arg.length()) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Bad -p command-line value (%s). Missing ':' char.\n",
               arg.c_str()));
    return -1;
  }

  if (pos == 0) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Bad -p command-line value (%s). "
               "':' char cannot be first char.\n",
               arg.c_str()));
    return -1;
  }

  if (pos == (arg.length() - 1)) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Bad -p command-line value  (%s) - "
               "':' char cannot be last char.\n",
               arg.c_str()));
    return -1;
  }

  // Parse the sub_id from left of ':' char, and remainder to right of ':'.
  ACE_TString sub_id_str(arg.c_str(), pos);
  this->sub_addr_str_ = arg.c_str() + pos + 1;

  // RepoIds are conventionally created and managed by the DCPSInfoRepo. Those
  // generated here are for the sole purpose of verifying internal behavior.
  OpenDDS::DCPS::RepoIdBuilder builder(sub_id_);

  builder.participantId(1);
  builder.entityKey(ACE_OS::atoi(sub_id_str.c_str()));
  builder.entityKind(OpenDDS::DCPS::ENTITYKIND_USER_WRITER_WITH_KEY);

  // Use the remainder as the "stringified" ACE_INET_Addr.
  this->sub_addr_ = ACE_INET_Addr(this->sub_addr_str_.c_str());

  return 0;
}
