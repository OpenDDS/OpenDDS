#include "SubDriver.h"
#include "TestException.h"
// Add the TransportImpl.h before TransportImpl_rch.h is included to
// resolve the build problem that the class is not defined when
// RcHandle<T> template is instantiated.
#include "dds/DCPS/transport/framework/TransportImpl.h"
#include "dds/DCPS/transport/simpleUnreliableDgram/SimpleUnreliableDgram.h"
#include "dds/DCPS/transport/simpleUnreliableDgram/SimpleUdpConfiguration.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include "dds/DCPS/transport/framework/NetworkAddress.h"
#include "dds/DCPS/AssociationData.h"
#include "dds/DCPS/RepoIdBuilder.h"
#include "dds/DCPS/Service_Participant.h"
#include "SimpleSubscriber.h"
#include <ace/Arg_Shifter.h>
#include <ace/OS.h>


SubDriver::SubDriver()
  : pub_id_ (OpenDDS::DCPS::GuidBuilder::create ()),
    sub_id_ (OpenDDS::DCPS::GuidBuilder::create ()),
    num_msgs_(100)
{
}


SubDriver::~SubDriver()
{
}


void
SubDriver::run(int& argc, char* argv[])
{
  // Need call the ORB_init to dynamically load the SimpleUdp library via
  // service configurator.
  // initialize the orb
  CORBA::ORB_var orb = CORBA::ORB_init (argc,
                                        argv,
                                        "TAO_DDS_DCPS");
  TheServiceParticipant->set_ORB (orb.in());
  DDS::DomainParticipantFactory_var dpf;
  dpf = TheParticipantFactoryWithArgs(argc, argv);

  parse_args(argc, argv);
  init();
  run();
}


void
SubDriver::parse_args(int& argc, char* argv[])
{
  // Command-line arguments:
  //
  // -p <pub_id:pub_host:pub_port>
  // -s <sub_id:pub_host:sub_port>
  // -n <num_messages>
  //
  ACE_Arg_Shifter arg_shifter(argc, argv);

  bool got_n = false;
  bool got_p = false;
  bool got_s = false;

  const char* current_arg = 0;

  while (arg_shifter.is_anything_left())
  {
    // The '-n' option
    if ((current_arg = arg_shifter.get_the_parameter("-n"))) {
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
    // The '-p' option
    if ((current_arg = arg_shifter.get_the_parameter("-p"))) {
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
    else if ((current_arg = arg_shifter.get_the_parameter("-s"))) {
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
    // The '-?' option
    else if (arg_shifter.cur_arg_strncasecmp("-?") == 0) {
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
  // Now we can ask TheTransportFactory to create a TransportImpl object
  // using the SimpleUdp factory.  We also supply an identifier for this
  // particular TransportImpl object that will be created.  This is known
  // as the "impl_id", or "the TransportImpl's instance id".  The point is
  // that we assign the impl_id, and TheTransportFactory caches a reference
  // to the newly created TransportImpl object using the impl_id (ALL_TRAFFIC
  // in our case) as a key to the cache map.  Other parts of this client
  // application code will be able use the obtain() method on
  // TheTransportFactory, provide the impl_id (ALL_TRAFFIC in our case), and
  // a reference to the cached TransportImpl will be returned.
  OpenDDS::DCPS::TransportImpl_rch transport_impl
    = TheTransportFactory->create_transport_impl (ALL_TRAFFIC,
                                                  "SimpleUdp",
                                                  OpenDDS::DCPS::DONT_AUTO_CONFIG);

  // Get the existing or create a new SimpleUdpConfiguration object.  It just has one field
  // to set - the local_address_ field.  This is the address that will be
  // used to open an acceptor object to listen for passive connection
  // requests.  This is the TransportImpl object's (local) "endpoint" address.
  // See comments in the $TAO_ROOT/orbsvcs/tests/DDS/transport/simple/
  // PubDriver.cpp (in the PubDriver::init() method) that describes the
  // other configuration options available.
  OpenDDS::DCPS::TransportConfiguration_rch config
    = TheTransportFactory->create_configuration (ALL_TRAFFIC, "SimpleUdp");

  OpenDDS::DCPS::SimpleUdpConfiguration* udp_config
    = static_cast <OpenDDS::DCPS::SimpleUdpConfiguration*> (config.in ());

  udp_config->local_address_ = this->sub_addr_;
  udp_config->local_address_str_ = this->sub_addr_str_;

  // Supply the config object to the TranportImpl object via its configure()
  // method.
  if (transport_impl->configure(config.in()) != 0)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) Failed to configure the transport impl\n"));
      throw TestException();
    }

  // And we are done with the init().
}


void
SubDriver::run()
{
  // Set up the publications.
  OpenDDS::DCPS::AssociationData publications[1];
  publications[0].remote_id_                = this->pub_id_;
  publications[0].remote_data_.transport_id = 2; // TBD later - wrong
  publications[0].remote_data_.publication_transport_priority = 0;

  OpenDDS::DCPS::NetworkAddress network_order_address(this->pub_addr_str_);

  ACE_OutputCDR cdr;
  cdr << network_order_address;
  size_t len = cdr.total_length ();

  publications[0].remote_data_.data
    = OpenDDS::DCPS::TransportInterfaceBLOB
    (len,
    len,
    (CORBA::Octet*)(cdr.buffer ()));

  // Write a file so that test script knows we're ready
  FILE * file = ACE_OS::fopen ("subready.txt", "w");
  ACE_OS::fprintf (file, "Ready\n");
  ACE_OS::fclose (file);

  this->subscriber_.init(ALL_TRAFFIC,
                         this->sub_id_,
                         1,               /* size of publications array */
                         publications,
                         this->num_msgs_);

  // Wait until we receive our expected message from the remote
  // publisher.  For this test, we should wait until we receive the
  // "Hello World!" message that we expect.  Then this program
  // can just shutdown.
  while (this->subscriber_.received_test_message() == 0)
    {
      ACE_OS::sleep(1);
    }

  // Tear-down the entire Transport Framework.
  TheTransportFactory->release();
  TheServiceParticipant->shutdown();
}


int
SubDriver::parse_pub_arg(const std::string& arg)
{
  std::string::size_type pos;

  // Find the first ':' character, and make sure it is in a legal spot.
  if ((pos = arg.find_first_of(':')) == std::string::npos) {
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
  std::string pub_id_str(arg,0,pos);
  this->pub_addr_str_ = std::string (arg,pos+1,std::string::npos); //use 3-arg constructor to build with VC6
  
  // RepoIds are conventionally created and managed by the DCPSInfoRepo. Those
  // generated here are for the sole purpose of verifying internal behavior.
  OpenDDS::DCPS::RepoIdBuilder builder(pub_id_);

  builder.participantId(1);
  builder.entityKey(ACE_OS::atoi(pub_id_str.c_str()));
  builder.entityKind(OpenDDS::DCPS::ENTITYKIND_USER_WRITER_WITH_KEY);

  // Find the (only) ':' char in the remainder, and make sure it is in
  // a legal spot.
  if ((pos = this->pub_addr_str_.find_first_of(':')) == std::string::npos) {
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
SubDriver::parse_sub_arg(const std::string& arg)
{
  std::string::size_type pos;

  // Find the first ':' character, and make sure it is in a legal spot.
  if ((pos = arg.find_first_of(':')) == std::string::npos) {
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
  std::string sub_id_str(arg,0,pos);
  this->sub_addr_str_ = std::string (arg,pos+1,std::string::npos); //use 3-arg constructor to build with VC6

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
