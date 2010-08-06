// -*- C++ -*-
//
// $Id$

#include "SubDriver.h"
#include "TestException.h"
#include "dds/DCPS/RepoIdBuilder.h"
// Add the TransportImpl.h before TransportImpl_rch.h is included to
// resolve the build problem that the class is not defined when
// RcHandle<T> template is instantiated.
#include "dds/DCPS/transport/framework/TransportImpl.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include <ace/Arg_Shifter.h>
#include <string>


SubDriver::SubDriver()
{
}


SubDriver::~SubDriver()
{
}


void
SubDriver::run(int& argc, ACE_TCHAR* argv[])
{
  this->parse_args(argc, argv);
  this->init();
  this->run();
}


void
SubDriver::parse_args(int& argc, ACE_TCHAR* argv[])
{
  // Command-line arguments:
  //
  // -n <num samples to send>
  // -d <data size>
  // -p <pub_id:pub_host:pub_port>
  // -s <sub_id:sub_host:sub_port>
  //

  ACE_Arg_Shifter arg_shifter(argc, argv);

  bool flag_n = false;
  bool flag_d = false;
  bool flag_p = false;
  bool flag_s = false;

  const ACE_TCHAR* current_arg = 0;

  while (arg_shifter.is_anything_left())
    {
      if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-n"))))
        {
          this->parse_arg_n(current_arg, flag_n);
          arg_shifter.consume_arg();
        }
      else if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-d"))))
        {
          this->parse_arg_d(current_arg, flag_d);
          arg_shifter.consume_arg();
        }
      else if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-p"))))
        {
          this->parse_arg_p(current_arg, flag_p);
          arg_shifter.consume_arg();
        }
      else if ((current_arg = arg_shifter.get_the_parameter(ACE_TEXT("-s"))))
        {
          this->parse_arg_s(current_arg, flag_s);
          arg_shifter.consume_arg();
        }
      else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-?")) == 0)
        {
          this->print_usage(argv[0]);
          arg_shifter.consume_arg();
          throw TestException();
        }
      else
        {
          arg_shifter.ignore_arg();
        }
    }

  this->required_arg('n', flag_n);
  this->required_arg('d', flag_d);
  this->required_arg('p', flag_p);
  this->required_arg('s', flag_s);
}


void
SubDriver::init()
{
  OpenDDS::DCPS::TransportImpl_rch transport_impl
    = TheTransportFactory->create_transport_impl (TRANSPORT_IMPL_ID,
                                                  ACE_TEXT("SimpleTcp"),
                                                  OpenDDS::DCPS::DONT_AUTO_CONFIG);
  OpenDDS::DCPS::TransportConfiguration_rch config
    = TheTransportFactory->create_configuration (TRANSPORT_IMPL_ID, ACE_TEXT("SimpleTcp"));

  OpenDDS::DCPS::SimpleTcpConfiguration* tcp_config
    = static_cast <OpenDDS::DCPS::SimpleTcpConfiguration*> (config.in ());

  tcp_config->local_address_ = this->local_address_;
  tcp_config->local_address_str_ = this->sub_addr_str_;

  if (transport_impl->configure(config.in()) != 0)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) Failed to configure the transport impl\n"));
      throw TestException();
    }
}


void
SubDriver::run()
{
  this->subscriber_.init(TRANSPORT_IMPL_ID);
  this->subscriber_.wait();
  TheTransportFactory->release();
}



void
SubDriver::parse_arg_n(const ACE_TCHAR* arg, bool& flag)
{
  if (flag)
    {
      ACE_ERROR((LM_ERROR,
             "(%P|%t) Only one -n allowed on command-line.\n"));
      throw TestException();
    }

  int value = ACE_OS::atoi(arg);

  if (value <= 0)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) Value following -n option must be > 0.\n"));
      throw TestException();
    }

  this->subscriber_.set_num_to_receive(value);

  flag = true;
}


void
SubDriver::parse_arg_d(const ACE_TCHAR* arg, bool& flag)
{
  if (flag)
    {
      ACE_ERROR((LM_ERROR,
             "(%P|%t) Only one -d allowed on command-line.\n"));
      throw TestException();
    }

  int value = ACE_OS::atoi(arg);

  if (value <= 0)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) Value following -d option must be > 0.\n"));
      throw TestException();
    }

  if (value > 32)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) Value following -d option must be < 32.\n"));
      throw TestException();
    }

  char data_size = value;
  this->subscriber_.set_data_size(data_size);

  flag = true;
}


void
SubDriver::parse_arg_p(const ACE_TCHAR* arg, bool& flag)
{
  if (flag)
    {
      ACE_ERROR((LM_ERROR,
             "(%P|%t) Only one -p allowed on command-line.\n"));
      throw TestException();
    }

  ACE_TString arg_str = arg;
  size_t pos;

  // Find the first ':' character, and make sure it is in a legal spot.
  if ((pos = std::find(arg_str.c_str(), arg_str.c_str() + arg_str.length(), ACE_TEXT(':')) - arg_str.c_str()) == arg_str.length())
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) Bad -p value (%s). "
                 "Missing ':' chars.\n",
                 arg));
      throw TestException();
    }

  if (pos == 0)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) Bad -p value (%s). "
                 "':' char cannot be first char.\n",
                 arg));
      throw TestException();
    }

  if (pos == arg_str.length() - 1)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) Bad -p value (%s). "
                 "':' char cannot be last char.\n",
                 arg));
      throw TestException();
    }

  // Parse the pub_id from left of ':' char, and remainder to right of ':'.
  ACE_TString pub_id_str(arg_str.c_str(), pos);
  ACE_TString pub_addr_str(arg_str.c_str() + pos + 1);

  // RepoIds are conventionally created and managed by the DCPSInfoRepo. Those
  // generated here are for the sole purpose of verifying internal behavior.
  OpenDDS::DCPS::RepoIdBuilder builder;

  builder.participantId(1);
  builder.entityKey(ACE_OS::atoi(pub_id_str.c_str()));
  builder.entityKind(OpenDDS::DCPS::ENTITYKIND_USER_WRITER_WITH_KEY);

  OpenDDS::DCPS::RepoId repoId(builder);

  ACE_INET_Addr pub_addr(pub_addr_str.c_str());

  subscriber_.add_remote_publisher(repoId, pub_addr, pub_addr_str);

  flag = true;
}


void
SubDriver::parse_arg_s(const ACE_TCHAR* arg, bool& flag)
{
  if (flag)
    {
      ACE_ERROR((LM_ERROR,
             "(%P|%t) Only one -s allowed on command-line.\n"));
      throw TestException();
    }

  ACE_TString arg_str = arg;
  size_t pos;

  // Find the first ':' character, and make sure it is in a legal spot.
  if ((pos = std::find(arg_str.c_str(), arg_str.c_str() + arg_str.length(), ACE_TEXT(':')) - arg_str.c_str()) == arg_str.length())
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) Bad -s value (%s). "
                 "Missing ':' chars.\n",
                 arg));
      throw TestException();
    }

  if (pos == 0)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) Bad -s value (%s). "
                 "':' char cannot be first char.\n",
                 arg));
      throw TestException();
    }

  if (pos == arg_str.length() - 1)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) Bad -s value (%s). "
                 "':' char cannot be last char.\n",
                 arg));
      throw TestException();
    }

  // Parse the sub_id from left of ':' char, and remainder to right of ':'.
  ACE_TString sub_id_str(arg_str.c_str() ,pos);
  this->sub_addr_str_ = arg_str.c_str() + pos + 1;

  // RepoIds are conventionally created and managed by the DCPSInfoRepo. Those
  // generated here are for the sole purpose of verifying internal behavior.
  OpenDDS::DCPS::RepoIdBuilder builder;

  builder.participantId(1);
  builder.entityKey(ACE_OS::atoi(sub_id_str.c_str()));
  builder.entityKind(OpenDDS::DCPS::ENTITYKIND_USER_WRITER_WITH_KEY);

  OpenDDS::DCPS::RepoId repoId(builder);

  this->local_address_ = ACE_INET_Addr(this->sub_addr_str_.c_str());

  subscriber_.set_local_subscriber(repoId);

  flag = true;
}


void
SubDriver::print_usage(const ACE_TCHAR* exe_name)
{
  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("Usage for executable: %s\n\n    -n num_msgs_to_send\n")
             ACE_TEXT("    -d data_size\n    -p pub_id:pub_host:pub_port\n")
             ACE_TEXT("    -s sub_id:sub_host:sub_port\n"),
             exe_name));
}


void
SubDriver::required_arg(ACE_TCHAR opt, bool flag)
{
  if (!flag)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) Missing required command-line option: -%c.\n",
                 opt));
      throw TestException();
    }
}
