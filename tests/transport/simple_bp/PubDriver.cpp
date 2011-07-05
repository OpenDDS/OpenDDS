#include "PubDriver.h"
#include "TestException.h"
// Add the TransportImpl.h before TransportImpl_rch.h is included to
// resolve the build problem that the class is not defined when
// RcHandle<T> template is instantiated.
#include "dds/DCPS/transport/framework/TransportImpl.h"
#include "dds/DCPS/transport/tcp/TcpInst.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include "dds/DCPS/transport/framework/NetworkAddress.h"
#include "dds/DCPS/AssociationData.h"
#include "dds/DCPS/RepoIdBuilder.h"
#include "dds/DCPS/Service_Participant.h"
#include "SimplePublisher.h"
#include <ace/Arg_Shifter.h>


PubDriver::PubDriver()
: pub_id_ (OpenDDS::DCPS::GuidBuilder::create ()),
  sub_id_ (OpenDDS::DCPS::GuidBuilder::create ()),
  num_msgs_(10000),
  msg_size_(128)
{
}


PubDriver::~PubDriver()
{
}


void
PubDriver::run(int& argc, ACE_TCHAR* argv[])
{
  CORBA::ORB_var orb = CORBA::ORB_init (argc,
                                        argv,
                                        OpenDDS::DCPS::DEFAULT_ORB_NAME);

  TheServiceParticipant->set_ORB (orb.in());
  DDS::DomainParticipantFactory_var dpf;
  dpf = TheParticipantFactoryWithArgs(argc, argv);

  parse_args(argc, argv);
  init();
  run();
}


void
PubDriver::parse_args(int& argc, ACE_TCHAR* argv[])
{
  // Command-line arguments:
  //
  // -p <pub_id:pub_port>
  // -s <sub_id:sub_host:sub_port>
  // -n num_messages
  // -c message_size
  //
  ACE_Arg_Shifter arg_shifter(argc, argv);

  bool got_n = false;
  bool got_p = false;
  bool got_s = false;
  bool got_c = false;

  const ACE_TCHAR* current_arg = 0;

  while (arg_shifter.is_anything_left())
  {
    // The '-n' option
    if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-n")))) {
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
    else if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-p")))) {
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
    // The '-c' option
    else if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-c")))) {
      if (got_c) {
        ACE_ERROR((LM_ERROR,
                   "(%P|%t) Only one -c allowed on command-line.\n"));
        throw TestException();
      }

      int value = ACE_OS::atoi(current_arg);
      arg_shifter.consume_arg();

      if (value <= 0) {
        ACE_ERROR((LM_ERROR,
                   "(%P|%t) Value following -c option must be > 0.\n"));
        throw TestException();
      }

      this->msg_size_ = value;

      got_n = true;
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
PubDriver::init()
{
  OpenDDS::DCPS::TransportImpl_rch transport_impl
    = TheTransportFactory->create_transport_impl (ALL_TRAFFIC,
                                                  ACE_TEXT("tcp"),
                                                  OpenDDS::DCPS::DONT_AUTO_CONFIG);
  OpenDDS::DCPS::TransportInst_rch config
    = TheTransportFactory->create_configuration (ALL_TRAFFIC, ACE_TEXT("tcp"));

  OpenDDS::DCPS::TcpInst* tcp_config
    = static_cast <OpenDDS::DCPS::TcpInst*> (config.in ());

  tcp_config->local_address_ = this->pub_addr_;
  tcp_config->local_address_str_ = this->pub_addr_str_.c_str();

  if (transport_impl->configure(config.in()) != 0)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) Failed to configure the transport impl\n"));
      throw TestException();
    }
}


void
PubDriver::run()
{
  // Set up the subscriptions.
  OpenDDS::DCPS::AssociationData subscriptions[1];
  subscriptions[0].remote_id_                = this->sub_id_;
  subscriptions[0].remote_data_.transport_id = 1;  // TBD - not right
  subscriptions[0].remote_data_.publication_transport_priority = 0;

  OpenDDS::DCPS::NetworkAddress network_order_address(this->sub_addr_str_.c_str());

  ACE_OutputCDR cdr;
  cdr << network_order_address;
  size_t len = cdr.total_length ();

  subscriptions[0].remote_data_.data
    = OpenDDS::DCPS::TransportInterfaceBLOB
    (len,
    len,
    (CORBA::Octet*)(cdr.buffer ()));

  this->publisher_.init(ALL_TRAFFIC,
                        this->pub_id_,
                        1,               /* size of subscriptions array */
                        subscriptions);

  // Wait for a fully association establishment and then start sending samples.
  ACE_OS::sleep (2);

  this->publisher_.run(this->num_msgs_, this->msg_size_);

  while (this->publisher_.delivered_test_message() == 0)
    {
      ACE_OS::sleep(1);
    }

  // TBD: Do we need a shutdown message from subscriber ?
  // Wait for subsciber to receive messages and then close the connection.
  // Increase the time when more messages are sent.
  //ACE_OS::sleep (5);

  OpenDDS::DCPS::ReaderIdSeq readers;
  readers.length(1);
  readers[0] = this->sub_id_;

  this->publisher_.remove_associations(1, readers.get_buffer(), this->pub_id_);

  // Tear-down the entire Transport Framework.
  TheTransportFactory->release();
  TheServiceParticipant->shutdown();
}


int
PubDriver::parse_pub_arg(const ACE_TString& arg)
{
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

  this->pub_addr_ = ACE_INET_Addr(this->pub_addr_str_.c_str());

  return 0;
}


int
PubDriver::parse_sub_arg(const ACE_TString& arg)
{
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

  // Find the (only) ':' char in the remainder, and make sure it is in
  // a legal spot.
  if ((pos = this->sub_addr_str_.find(ACE_TEXT(':'))) == std::string::npos) {
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
  this->sub_addr_ = ACE_INET_Addr(this->sub_addr_str_.c_str());

  return 0;
}
