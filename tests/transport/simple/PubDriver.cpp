#include "PubDriver.h"
#include "TestException.h"
// Add the TransportImpl.h before TransportImpl_rch.h is included to
// resolve the build problem that the class is not defined when
// RcHandle<T> template is instantiated.
#include "dds/DCPS/transport/framework/TransportImpl.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include "dds/DCPS/transport/framework/NetworkAddress.h"
#include "dds/DCPS/AssociationData.h"
#include "dds/DCPS/RepoIdBuilder.h"
#include "dds/DCPS/Service_Participant.h"
#include "SimplePublisher.h"
#include <ace/Arg_Shifter.h>

#include "dds/DCPS/transport/framework/EntryExit.h"


PubDriver::PubDriver()
: pub_id_ (OpenDDS::DCPS::GuidBuilder::create ()),
  sub_id_ (OpenDDS::DCPS::GuidBuilder::create ())
{
  DBG_ENTRY("PubDriver","PubDriver");
}


PubDriver::~PubDriver()
{
  DBG_ENTRY("PubDriver","~PubDriver");
}


void
PubDriver::run(int& argc, ACE_TCHAR* argv[])
{
  DBG_ENTRY_LVL("PubDriver","run",6);

  CORBA::ORB_var orb = CORBA::ORB_init (argc,
                                        argv,
                                        OpenDDS::DCPS::DEFAULT_ORB_NAME);

  TheServiceParticipant->set_ORB (orb.in());
  DDS::DomainParticipantFactory_var dpf;
  dpf = TheParticipantFactoryWithArgs(argc, argv);

  this->parse_args(argc, argv);
  this->init();
  this->run();
}


void
PubDriver::parse_args(int& argc, ACE_TCHAR* argv[])
{
  DBG_ENTRY("PubDriver","parse_args");

  // Command-line arguments:
  //
  // -p <pub_id:pub_port>
  // -s <sub_id:sub_host:sub_port>
  //
  ACE_Arg_Shifter arg_shifter(argc, argv);

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
  DBG_ENTRY("PubDriver","init");

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Use TheTransportFactory to create a TransportImpl object "
             "of SimpleTcp with the ALL_TRAFFIC transport_id (%d).\n",
             ALL_TRAFFIC));

  OpenDDS::DCPS::TransportImpl_rch transport_impl
    = TheTransportFactory->create_transport_impl (ALL_TRAFFIC,
                                                  ACE_TEXT("SimpleTcp"),
                                                  OpenDDS::DCPS::DONT_AUTO_CONFIG);

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Get an existing or create a new SimpleTcpConfiguration object.\n"));

  OpenDDS::DCPS::TransportConfiguration_rch config
    = TheTransportFactory->create_configuration (ALL_TRAFFIC, ACE_TEXT("SimpleTcp"));

  OpenDDS::DCPS::SimpleTcpConfiguration* tcp_config
    = static_cast <OpenDDS::DCPS::SimpleTcpConfiguration*> (config.in ());

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Set the config->local_address_ to our (local) pub_addr_.\n"));

  tcp_config->local_address_ = this->pub_addr_;
  tcp_config->local_address_str_ = this->pub_addr_str_;

  // Here are the other config settings that I could change (they have
  // default values if I don't change them here):
  //
  //    config->swap_bytes_
  //
  //        Value must be 0 or 1.  Default value is 0.
  //
  //    config->queue_messages_per_pool_
  //
  //        Used to pre-allocate the "linked-list" objects for each
  //        TransportSendStrategy object's (send) queue.
  //
  //        Value must be greater than 0.  Default value is 10.
  //
  //    config->queue_initial_pools_
  //
  //        Used to pre-allocate the "linked-list" objects for each
  //        TransportSendStrategy object's (send) queue.
  //
  //        Value must be greater than 0.  Default value is 5.
  //
  //    config->max_packet_size_
  //
  //        Absolute largest size of any (send) transport packet.
  //
  //        Value must be greater than the marshalled size of the
  //        TransportHeader (the packet header).  Value must also be
  //        less than 65536.  Default value is 65535.
  //
  //    config->max_samples_per_packet_
  //
  //        Absolute maximum number of samples that can belong to a
  //        single (send) transport packet.
  //
  //        Value must be greater than 0.  Defaults to 10.
  //
  //    config->optimum_packet_size_
  //
  //        If possible, the transport will try to build transport packets
  //        that are at least the optimum_packet_size_.
  //
  //        Any value allowed.  Defaults to 4096.
  //
  //    config->set_send_thread_strategy(ThreadSynchStrategy* strategy)
  //
  //        TBD - Update these comments once I've changed ThreadSynchStrategy.
  //        This is used to change the ThreadSynchStrategy object.
  //        Note that this is going to change soon such that the
  //        ThreadSynchStrategy object will be reference-counted, and
  //        the configuration object will have a smart pointer data member
  //        in place of the existing raw pointer.  This will eliminate
  //        the need for this method, and you will just assign a new
  //        ThreadSynchStrategy object just like other fields in the config.
  //
  //        Default ThreadSynchStrategy is a PerConnectionSynchStrategy object.
  //

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Configure the (ALL_TRAFFIC) TransportImpl object.\n"));

  // Pass the configuration object to the transport_impl's configure()
  // method as an "in" argument.  Don't change the configuration object
  // after this has been done since it will cause unknown results.
  if (transport_impl->configure(config.in()) != 0)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) Failed to configure the transport impl\n"));
      throw TestException();
    }

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "The TransportImpl object has been successfully configured.\n"));
}


void
PubDriver::run()
{
  DBG_ENTRY_LVL("PubDriver","run",6);

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Create the 'subscriptions' (array of AssociationData).\n"));

  // Set up the subscriptions.
  OpenDDS::DCPS::AssociationData subscriptions[1];
  subscriptions[0].remote_id_                = this->sub_id_;
  subscriptions[0].remote_data_.transport_id = 1;  // TBD - not right
  subscriptions[0].remote_data_.publication_transport_priority = 0;

  OpenDDS::DCPS::NetworkAddress network_order_address(this->sub_addr_str_);

  ACE_OutputCDR cdr;
  cdr << network_order_address;
  size_t len = cdr.total_length ();

  subscriptions[0].remote_data_.data
    = OpenDDS::DCPS::TransportInterfaceBLOB
    (len,
    len,
    (CORBA::Octet*)(cdr.buffer ()));

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Initialize our SimplePublisher object.\n"));

  this->publisher_.init(ALL_TRAFFIC,
                        this->pub_id_,
                        1,               /* size of subscriptions array */
                        subscriptions);

  // Wait for a fully association establishment and then start sending samples.
  ACE_OS::sleep (2);

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Run our SimplePublisher object.\n"));

  this->publisher_.run();

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Ask the SimplePublisher object if it is done running. "
             "If not, sleep for 1 second, and ask again.\n"));

  while (this->publisher_.delivered_test_message() == 0)
    {
      ACE_OS::sleep(1);
    }

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "The SimplePublisher object is done running.  "
             "Release TheTransportFactory - causing all TransportImpl "
             "objects to be shutdown().\n"));

  OpenDDS::DCPS::ReaderIdSeq readers;
  readers.length(1);
  readers[0] = this->sub_id_;

  this->publisher_.remove_associations(1, readers.get_buffer(), this->pub_id_);

  // Tear-down the entire Transport Framework.
  TheTransportFactory->release();
  TheServiceParticipant->shutdown();

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "TheTransportFactory has finished releasing.\n"));
}


int
PubDriver::parse_pub_arg(const ACE_TString& arg)
{
  DBG_ENTRY("PubDriver","parse_pub_arg");

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
  DBG_ENTRY("PubDriver","parse_sub_arg");

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
  if ((pos = std::find(sub_addr_str_.c_str(), sub_addr_str_.c_str() + sub_addr_str_.length(), ACE_TEXT(':')) - sub_addr_str_.c_str()) == sub_addr_str_.length()) {
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
