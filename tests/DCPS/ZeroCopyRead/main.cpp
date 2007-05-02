// -*- C++ -*-
// ============================================================================
/**
 *  @file   main.cpp
 *
 *  $Id$
 *
 *  Test of zero-copy read.
 */
// ============================================================================


#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/Qos_Helper.h"
#include "dds/DCPS/TopicDescriptionImpl.h"
#include "dds/DCPS/SubscriberImpl.h"
#include "dds/DCPS/PublisherImpl.h"
#include "SimpleTypeSupportImpl.h"
#include "dds/DCPS/transport/framework/EntryExit.h"

#include "dds/DCPS/transport/simpleUnreliableDgram/SimpleUdpConfiguration.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"

#include "ace/Arg_Shifter.h"

#include <string>

class TestException
{
  public:

    TestException()  {}
    ~TestException() {}
};


const long  MY_DOMAIN   = 411;
const char* MY_TOPIC    = "foo";
const char* MY_TYPE     = "foo";

const ACE_Time_Value max_blocking_time(::DDS::DURATION_INFINITY_SEC);

int use_take = 0;
int multiple_instances = 0;
int max_samples_per_instance = ::DDS::LENGTH_UNLIMITED;
int history_depth = 1 ;
bool support_client_side_BIT = false;

int test_failed = 0;

TAO::DCPS::TransportImpl_rch reader_transport_impl;
TAO::DCPS::TransportImpl_rch writer_transport_impl;

enum TransportInstanceId
{
  SUB_TRAFFIC,
  PUB_TRAFFIC
};


/// This is an allocator that simply uses malloc and free.
/// A real allocator would most likely use a pool of memory.
template<class T, std::size_t N>
class BogusExampleAllocator : public ACE_Allocator
{
public:
    BogusExampleAllocator()
        : num_allocs_(0)
        , num_frees_(0)
    {};

    int num_allocs() { return num_allocs_;};
    int num_frees()  { return num_frees_;};

    virtual void *malloc (size_t nbytes) { 
        num_allocs_++;
        return ACE_OS::malloc(nbytes);
    };
    virtual void free (void *ptr) {
        num_frees_++;
        ACE_OS::free(ptr);
    };

    void *calloc (size_t nbytes, char initial_value) 
            {/* no-op */ 
              ACE_UNUSED_ARG (nbytes);
              ACE_UNUSED_ARG (initial_value);
              return (void*)0;
            };

    void *calloc (size_t n_elem, size_t elem_size, char initial_value)
            {/* no-op */ 
              ACE_UNUSED_ARG (n_elem);
              ACE_UNUSED_ARG (elem_size);
              ACE_UNUSED_ARG (initial_value);
              return (void*)0;
            };

    int remove (void)
            {/* no-op */ 
                ACE_ASSERT("not supported" ==0);
                return -1; 
            };

    int bind (const char *name, void *pointer, int duplicates)
            {/* no-op */ 
                ACE_ASSERT("not supported" ==0);
                ACE_UNUSED_ARG (name);
                ACE_UNUSED_ARG (pointer);
                ACE_UNUSED_ARG (duplicates);
                return -1; 
            };

    int trybind (const char *name, void *&pointer)
            {/* no-op */ 
                ACE_ASSERT("not supported" ==0);
                ACE_UNUSED_ARG (name);
                ACE_UNUSED_ARG (pointer);
                return -1; 
            };

    int find (const char *name, void *&pointer)
            {/* no-op */ 
                ACE_ASSERT("not supported" ==0);
                ACE_UNUSED_ARG (name);
                ACE_UNUSED_ARG (pointer);
                return -1; 
            };

    int find (const char *name)
            {/* no-op */ 
                ACE_ASSERT("not supported" ==0);
                ACE_UNUSED_ARG (name);
                return -1; 
            };

    int unbind (const char *name)
            {/* no-op */ 
                ACE_ASSERT("not supported" ==0);
                ACE_UNUSED_ARG (name);
                return -1; 
            };

    int unbind (const char *name, void *&pointer)
            {/* no-op */ 
                ACE_ASSERT("not supported" ==0);
                ACE_UNUSED_ARG (name);
                ACE_UNUSED_ARG (pointer);
                return -1; 
            };

    int sync (ssize_t len, int flags)
            {/* no-op */ 
                ACE_ASSERT("not supported" ==0);
                ACE_UNUSED_ARG (len);
                ACE_UNUSED_ARG (flags);
                return -1; 
            };

    int sync (void *addr, size_t len, int flags)
            {/* no-op */ 
                ACE_ASSERT("not supported" ==0);
                ACE_UNUSED_ARG (addr);
                ACE_UNUSED_ARG (len);
                ACE_UNUSED_ARG (flags);
                return -1; 
            };

    int protect (ssize_t len, int prot)
            {/* no-op */ 
                ACE_ASSERT("not supported" ==0);
                ACE_UNUSED_ARG (len);
                ACE_UNUSED_ARG (prot);
                return -1; 
            };

    int protect (void *addr, size_t len, int prot)
            {/* no-op */ 
                ACE_ASSERT("not supported" ==0);
                ACE_UNUSED_ARG (addr);
                ACE_UNUSED_ARG (len);
                ACE_UNUSED_ARG (prot);
                return -1; 
            };

#if defined (ACE_HAS_MALLOC_STATS)

    void print_stats (void) const
            {/* no-op */ 
                ACE_ASSERT("not supported" ==0);
            };
#endif /* ACE_HAS_MALLOC_STATS */

    void dump (void) const
            {/* no-op */ 
                ACE_ASSERT("not supported" ==0);
            };

private:
    // do not allow copies. - I am not sure this restriction is necessary.
    BogusExampleAllocator(const BogusExampleAllocator&);
    BogusExampleAllocator& operator=(const BogusExampleAllocator&);

    int num_allocs_;
    int num_frees_;
};


int init_tranport ()
{
  int status = 0;

      reader_transport_impl
        = TheTransportFactory->create_transport_impl (SUB_TRAFFIC,
                                                      "SimpleTcp",
                                                      TAO::DCPS::DONT_AUTO_CONFIG);

      TAO::DCPS::TransportConfiguration_rch reader_config
        = TheTransportFactory->create_configuration (SUB_TRAFFIC, "SimpleTcp");


      if (reader_transport_impl->configure(reader_config.in()) != 0)
        {
          ACE_ERROR((LM_ERROR,
                    ACE_TEXT("(%P|%t) init_transport: sub TCP ")
                    ACE_TEXT(" Failed to configure the transport.\n")));
          status = 1;
        }

      writer_transport_impl
        = TheTransportFactory->create_transport_impl (PUB_TRAFFIC,
                                                      "SimpleTcp",
                                                      TAO::DCPS::DONT_AUTO_CONFIG);
      TAO::DCPS::TransportConfiguration_rch writer_config
        = TheTransportFactory->create_configuration (PUB_TRAFFIC, "SimpleTcp");

      if (writer_transport_impl->configure(writer_config.in()) != 0)
        {
          ACE_ERROR((LM_ERROR,
                    ACE_TEXT("(%P|%t) init_transport: sub TCP")
                    ACE_TEXT(" Failed to configure the transport.\n")));
          status = 1;
        }

  return status;
}


int wait_for_data (::DDS::Subscriber_ptr sub,
                   int timeout_sec)
{
  const int factor = 10;
  ACE_Time_Value small(0,1000000/factor);
  int timeout_loops = timeout_sec * factor;

  ::DDS::DataReaderSeq_var discard = new ::DDS::DataReaderSeq(10);
  while (timeout_loops-- > 0)
    {
      sub->get_datareaders (
                    discard.out (),
                    ::DDS::NOT_READ_SAMPLE_STATE,
                    ::DDS::ANY_VIEW_STATE,
                    ::DDS::ANY_INSTANCE_STATE );
      if (discard->length () > 0)
        return 1;

      ACE_OS::sleep (small);
    }
  return 0;
}

/// parse the command line arguments
int parse_args (int argc, char *argv[])
{

  u_long mask =  ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS) ;
  ACE_LOG_MSG->priority_mask(mask | LM_TRACE | LM_DEBUG, ACE_Log_Msg::PROCESS) ;
  ACE_Arg_Shifter arg_shifter (argc, argv);

  while (arg_shifter.is_anything_left ())
  {
    // options:
    //  -n max_samples_per_instance defaults to INFINITE
    //  -d history.depth            defaults to 1
    //  -z                          verbose transport debug
    //  -b                          enable client side Built-In topic support

    const char *currentArg = 0;

    if ((currentArg = arg_shifter.get_the_parameter("-n")) != 0)
    {
      max_samples_per_instance = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter("-d")) != 0)
    {
      history_depth = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if (arg_shifter.cur_arg_strncasecmp("-z") == 0)
    {
      TURN_ON_VERBOSE_DEBUG;
      arg_shifter.consume_arg();
    }
    else if (arg_shifter.cur_arg_strncasecmp("-b") == 0)
    {
      support_client_side_BIT = true;
      arg_shifter.consume_arg();
    }
    else
    {
      arg_shifter.ignore_arg ();
    }
  }
  // Indicates sucessful parsing of the command line
  return 0;
}

void check_read_status(DDS::ReturnCode_t status,
                       const SimpleZCSeq& data,
                       CORBA::ULong expected,
                       const char* where)
{

      if (status == ::DDS::RETCODE_OK)
      {
          if (data.length() != expected)
          {
              ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) %s ERROR: expected %d samples but got %d\n"),
                  where, expected, data.length() ));
              test_failed = 1;
              throw TestException();
          }

      }
      else if (status == ::DDS::RETCODE_NO_DATA)
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT("(%P|%t) %s ERROR: reader received NO_DATA!\n"), 
          where));
        test_failed = 1;
        throw TestException();
      }
      else if (status == ::DDS::RETCODE_PRECONDITION_NOT_MET)
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT("(%P|%t) %s ERROR: reader received PRECONDITION_NOT_MET!\n"), 
          where));
        test_failed = 1;
        throw TestException();
      }
      else
      {
        ACE_ERROR((LM_ERROR,
            ACE_TEXT("(%P|%t) %s ERROR: unexpected status %d!\n"),
            where, status ));
        test_failed = 1;
        throw TestException();
      }
}


void check_return_loan_status(DDS::ReturnCode_t status,
                       const SimpleZCSeq& data,
                       CORBA::ULong expected_len,
                       CORBA::ULong expected_max,
                       const char* where)
{

      if (status == ::DDS::RETCODE_OK)
      {
          if (data.length() != expected_len)
          {
              ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) %s ERROR: expected %d len but got %d\n"),
                  where, expected_len, data.length() ));
              test_failed = 1;
              throw TestException();
          }
          if (data.max_len() != expected_max)
          {
              ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) %s ERROR: expected %d max_len but got %d\n"),
                  where, expected_max, data.max_len() ));
              test_failed = 1;
              throw TestException();
          }

      }
      else if (status == ::DDS::RETCODE_NO_DATA)
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT("(%P|%t) %s ERROR: reader received NO_DATA!\n"), 
          where));
        test_failed = 1;
        throw TestException();
      }
      else if (status == ::DDS::RETCODE_PRECONDITION_NOT_MET)
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT("(%P|%t) %s ERROR: reader received PRECONDITION_NOT_MET!\n"), 
          where));
        test_failed = 1;
        throw TestException();
      }
      else
      {
        ACE_ERROR((LM_ERROR,
            ACE_TEXT("(%P|%t) %s ERROR: unexpected status %d!\n"),
            where, status ));
        test_failed = 1;
        throw TestException();
      }
}

int main (int argc, char *argv[])
{


  u_long mask =  ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS) ;
  ACE_LOG_MSG->priority_mask(mask | LM_TRACE | LM_DEBUG, ACE_Log_Msg::PROCESS) ;

  ACE_DEBUG((LM_DEBUG,"(%P|%t) zero-copy read test main\n"));
  try
    {
      ::DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);
      if (CORBA::is_nil (dpf.in ()))
      {
        ACE_ERROR ((LM_ERROR,
                   ACE_TEXT("(%P|%t) creating the DomainParticipantFactory failed.\n")));
        return 1 ;
      }

      // let the Service_Participant (in above line) strip out -DCPSxxx parameters
      // and then get application specific parameters.
      parse_args (argc, argv);

      SimpleTypeSupportImpl* sts_servant = new SimpleTypeSupportImpl();
      PortableServer::ServantBase_var safe_servant = sts_servant;

      SimpleTypeSupport_var fts =
        TAO::DCPS::servant_to_reference (sts_servant);

      ::DDS::DomainParticipant_var dp =
        dpf->create_participant(MY_DOMAIN,
                                PARTICIPANT_QOS_DEFAULT,
                                ::DDS::DomainParticipantListener::_nil());
      if (CORBA::is_nil (dp.in ()))
      {
        ACE_ERROR ((LM_ERROR,
                   ACE_TEXT("(%P|%t) create_participant failed.\n")));
        return 1 ;
      }

      if (::DDS::RETCODE_OK != fts->register_type(dp.in (), MY_TYPE))
        {
          ACE_ERROR ((LM_ERROR,
            ACE_TEXT ("Failed to register the SimpleTypeSupport.")));
          return 1;
        }


      ::DDS::TopicQos topic_qos;
      dp->get_default_topic_qos(topic_qos);

      topic_qos.history.kind = ::DDS::KEEP_LAST_HISTORY_QOS;
      topic_qos.history.depth = history_depth;
      topic_qos.resource_limits.max_samples_per_instance =
            max_samples_per_instance ;


      ::DDS::Topic_var topic =
        dp->create_topic (MY_TOPIC,
                          MY_TYPE,
                          topic_qos,
                          ::DDS::TopicListener::_nil());
      if (CORBA::is_nil (topic.in ()))
      {
        return 1 ;
      }

      ::DDS::TopicDescription_var description =
        dp->lookup_topicdescription(MY_TOPIC);
      if (CORBA::is_nil (description.in ()))
      {
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT("(%P|%t) lookup_topicdescription failed.\n")),
                           1);
      }



      // Create the subscriber
      ::DDS::Subscriber_var sub =
        dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                             ::DDS::SubscriberListener::_nil());
      if (CORBA::is_nil (sub.in ()))
      {
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT("(%P|%t) create_subscriber failed.\n")),
                           1);
      }

      // Create the publisher
      ::DDS::Publisher_var pub =
        dp->create_publisher(PUBLISHER_QOS_DEFAULT,
                             ::DDS::PublisherListener::_nil());
      if (CORBA::is_nil (pub.in ()))
      {
        ACE_ERROR_RETURN ((LM_ERROR,
                          ACE_TEXT("(%P|%t) create_publisher failed.\n")),
                          1);
      }

      // Initialize the transport
      if (0 != ::init_tranport() )
      {
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT("(%P|%t) init_transport failed!\n")),
                           1);
      }

      // Attach the subscriber to the transport.
      ::TAO::DCPS::SubscriberImpl* sub_impl
        = ::TAO::DCPS::reference_to_servant< ::TAO::DCPS::SubscriberImpl,
                                             ::DDS::Subscriber_ptr>
                              (sub.in ());

      if (0 == sub_impl)
      {
        ACE_ERROR_RETURN ((LM_ERROR,
                          ACE_TEXT("(%P|%t) Failed to obtain servant ::TAO::DCPS::SubscriberImpl\n")),
                          1);
      }

      sub_impl->attach_transport(reader_transport_impl.in());


      // Attach the publisher to the transport.
      ::TAO::DCPS::PublisherImpl* pub_impl
        = ::TAO::DCPS::reference_to_servant< ::TAO::DCPS::PublisherImpl,
                                             ::DDS::Publisher_ptr>
                              (pub.in ());

      if (0 == pub_impl)
      {
        ACE_ERROR_RETURN ((LM_ERROR,
                          ACE_TEXT("(%P|%t) Failed to obtain servant ::TAO::DCPS::PublisherImpl\n")),
                          1);
      }

      pub_impl->attach_transport(writer_transport_impl.in());

      // Create the datawriter
      ::DDS::DataWriterQos dw_qos;
      pub->get_default_datawriter_qos (dw_qos);

      dw_qos.history.kind = ::DDS::KEEP_LAST_HISTORY_QOS;
      dw_qos.history.depth = history_depth  ;
      dw_qos.resource_limits.max_samples_per_instance =
            max_samples_per_instance ;

      ::DDS::DataWriter_var dw = pub->create_datawriter(topic.in (),
                                        dw_qos,
                                        ::DDS::DataWriterListener::_nil());

      if (CORBA::is_nil (dw.in ()))
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT("(%P|%t) create_datawriter failed.\n")));
        return 1 ;
      }

      // Create the Datareader
      ::DDS::DataReaderQos dr_qos;
      sub->get_default_datareader_qos (dr_qos);

      dr_qos.history.kind = ::DDS::KEEP_LAST_HISTORY_QOS;
      dr_qos.history.depth = history_depth  ;
      dr_qos.resource_limits.max_samples_per_instance =
            max_samples_per_instance ;

      ::DDS::DataReader_var dr
        = sub->create_datareader(description.in (),
                                 dr_qos,
                                 ::DDS::DataReaderListener::_nil());

      if (CORBA::is_nil (dr.in ()))
        {
          ACE_ERROR_RETURN ((LM_ERROR,
            ACE_TEXT("(%P|%t) create_datareader failed.\n")),
            1);
        }

      SimpleDataWriter_var foo_dw
           = SimpleDataWriter::_narrow(dw.in ());
      if (CORBA::is_nil (foo_dw.in ()))
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT("(%P|%t) SimpleDataWriter::_narrow failed.\n")));
        return 1; // failure
      }

      SimpleDataWriterImpl* fast_dw =
        ::TAO::DCPS::reference_to_servant< SimpleDataWriterImpl,
                                           SimpleDataWriter_ptr>
                (foo_dw.in ());

      SimpleDataReader_var foo_dr
        = SimpleDataReader::_narrow(dr.in ());
      if (CORBA::is_nil (foo_dr.in ()))
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT("(%P|%t) SimpleDataReader::_narrow failed.\n")));
        return 1; // failure
      }

      SimpleDataReaderImpl* fast_dr =
        ::TAO::DCPS::reference_to_servant< SimpleDataReaderImpl,
                                           SimpleDataReader_ptr>
                (foo_dr.in ());


      // wait for association establishement before writing.
      // -- replaced this sleep with the while loop below; 
      //    waiting on the one association we expect.
      //  ACE_OS::sleep(5); //REMOVE if not needed
      ::DDS::InstanceHandleSeq handles;
      while (1)
      {
          fast_dw->get_matched_subscriptions(handles);
          if (handles.length() > 0)
              break;
          else
              ACE_OS::sleep(ACE_Time_Value(0,200000));
      }

      // =============== do the test ====


      ::DDS::OfferedIncompatibleQosStatus * incomp =
          foo_dw->get_offered_incompatible_qos_status ();

      int incompatible_transport_found = 0;
      for (CORBA::ULong ii =0; ii < incomp->policies.length (); ii++)
        {
          if (incomp->policies[ii].policy_id
                        == ::DDS::TRANSPORTTYPE_QOS_POLICY_ID)
            incompatible_transport_found = 1;
        }

      ::DDS::SubscriptionMatchStatus matched =
        foo_dr->get_subscription_match_status ();

      ::DDS::InstanceHandle_t handle;

          if (matched.total_count != 1)
            ACE_ERROR_RETURN((LM_ERROR,
              "TEST ERROR: expected subscription_match"
              " with count 1 but got %d\n",
              matched.total_count),
              9);

    try { // the real testing.
      ::Test::Simple foo;
      ::Test::MyLongSeq ls;
      //::Test::Simple::_ls_seq ls;
      ls.length(1);
      ls[0] = 5;
      foo.key  = 1;
      foo.count = 1;
      foo.text = CORBA::string_dup("t1");
      foo.ls = ls;
      

      handle
          = fast_dw->_cxx_register (foo);

      fast_dw->write(foo,
                     handle);


      // wait for new data for upto 5 seconds
      if (!wait_for_data(sub.in (), 5))
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT("(%P|%t) ERROR: timeout waiting for data.\n")),
                           1);

      
      TAO::DCPS::ReceivedDataElement *item;
      {
        //=====================================================
        // 1) show that zero-copy is zero-copy
        //=====================================================
        ACE_DEBUG((LM_INFO,"==== TEST 1 : show that zero-copy is zero-copy\n"));
        const CORBA::Long max_samples = 2;
        // 0 means zero-copy
        SimpleZCSeq                  data1 (0, max_samples);
        ::TAO::DCPS::SampleInfoZCSeq info1 (0,max_samples);

          
        DDS::ReturnCode_t status  ;
        status = fast_dr->read(  data1 
                                , info1
                                , max_samples
                                , ::DDS::ANY_SAMPLE_STATE
                                , ::DDS::ANY_VIEW_STATE
                                , ::DDS::ANY_INSTANCE_STATE );

          
        check_read_status(status, data1, 1, "t1 read2");

        // this should change the value returned by the next read
        data1[0].count = 999;

        SimpleZCSeq                  data2 (0, max_samples);
        ::TAO::DCPS::SampleInfoZCSeq info2 (0,max_samples);
        status = fast_dr->read(  data2 
                                , info2
                                , max_samples
                                , ::DDS::ANY_SAMPLE_STATE
                                , ::DDS::ANY_VIEW_STATE
                                , ::DDS::ANY_INSTANCE_STATE );

          
        check_read_status(status, data2, 1, "t1 read2");

        if (data1[0].count != data2[0].count)
        {
            ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) t1 ERROR: zero-copy failed.\n") ));
            test_failed = 1;

        }

        item = data2.getPtr(0);
        if (item->ref_count_ != 3)
        {
            ACE_ERROR ((LM_ERROR,
                ACE_TEXT("(%P|%t) t4 ERROR: bad ref count %d expecting 3\n"), item->ref_count_ ));
            test_failed = 1;
        }


        status = fast_dr->return_loan(data2, info2 );

        check_return_loan_status(status, data2, 0, 0, "t1 return_loan2");

        if (item->ref_count_ != 2)
        {
            ACE_ERROR ((LM_ERROR,
                ACE_TEXT("(%P|%t) t4 ERROR: bad ref count %d expecting 2\n"), item->ref_count_ ));
            test_failed = 1;
        }
        status = fast_dr->return_loan(data1, info1 );

        check_return_loan_status(status, data1, 0, 0, "t1 return_loan1");

        // just the instance container should have a reference.
        if (item->ref_count_ != 1)
        {
            ACE_ERROR ((LM_ERROR,
                ACE_TEXT("(%P|%t) t4 ERROR: bad ref count %d expecting 1\n"), item->ref_count_ ));
            test_failed = 1;
        }
      } // t1

      {
        //=====================================================
        // 2) show that single-copy is makes copies
        //=====================================================
        ACE_DEBUG((LM_INFO,"==== TEST 2 : show that single-copy is makes copies\n"));
          
        const CORBA::Long max_samples = 2;
        // types supporting zero-copy read 
        SimpleZCSeq                  data1 (max_samples);
        ::TAO::DCPS::SampleInfoZCSeq info1 (max_samples);

        DDS::ReturnCode_t status  ;
        status = fast_dr->read(  data1 
                                , info1
                                , max_samples
                                , ::DDS::ANY_SAMPLE_STATE
                                , ::DDS::ANY_VIEW_STATE
                                , ::DDS::ANY_INSTANCE_STATE );

          
        check_read_status(status, data1, 1, "t1 read2");

        // this should change the value returned by the next read
        data1[0].count = 888;
        data1[0].text = CORBA::string_dup("t2");

        SimpleZCSeq                  data2 (max_samples);
        ::TAO::DCPS::SampleInfoZCSeq info2 (max_samples);
        status = fast_dr->read(  data2 
                                , info2
                                , max_samples
                                , ::DDS::ANY_SAMPLE_STATE
                                , ::DDS::ANY_VIEW_STATE
                                , ::DDS::ANY_INSTANCE_STATE );

          
        check_read_status(status, data2, 1, "t2 read2");

        if (data1[0].count == data2[0].count)
        {
            ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) t2 ERROR: single-copy test failed for scalar.\n") ));
            test_failed = 1;

        }

        //ACE_DEBUG((LM_DEBUG,"%s != %s\n", data1[0].text.in(), data2[0].text.in() ));

        if (0 == strcmp(data1[0].text.in(), data2[0].text.in()))
        {
            ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) t2 ERROR: single-copy test failed for string.\n") ));
            test_failed = 1;

        }

        status = fast_dr->return_loan(  data2 
                                      , info2 );

        check_return_loan_status(status, data2, 1, max_samples, "t2 return_loan2");

        status = fast_dr->return_loan(  data1 
                                      , info1 );

        check_return_loan_status(status, data1, 1, max_samples, "t2 return_loan1");

        // END OF BLOCK destruction.
        // 4/24/07 Note: breakpoint in the ls sequence destructor 
        // (part of Simple sample type) showed it was called when this 
        // block went out of scope (because the data1 and data2 are destroyed).
        // Good!
        // 4/24/07 Note: breakpoint in ACE_Array destructor showed 
        // it was called for the ZeroCopyInfoSeq. Good!

      } // t2

      {
        //=====================================================
        // 3) Show that zero-copy reference counting works.
        //    The zero-copy sequence will hold a reference
        //    while the instance container loses its
        //    reference because of history.depth.
        //=====================================================
        ACE_DEBUG((LM_INFO,"==== TEST 3 : Show that zero-copy reference counting works\n"));

        const CORBA::Long max_samples = 2;
        // 0 means zero-copy
        SimpleZCSeq                  data1 (0, max_samples);
        ::TAO::DCPS::SampleInfoZCSeq info1 (0,max_samples);

              
        foo.key  = 1;
        foo.count = 1;

        // since depth=1 the previous sample will be "lost"
        // from the instance container.
        fast_dw->write(foo,
                        handle);

        // wait for write to propogate
        if (!wait_for_data(sub.in (), 5))
            ACE_ERROR_RETURN ((LM_ERROR,
                            ACE_TEXT("(%P|%t) t3 ERROR: timeout waiting for data.\n")),
                            1);

        DDS::ReturnCode_t status  ;
        status = fast_dr->read(  data1 
                                , info1
                                , max_samples
                                , ::DDS::ANY_SAMPLE_STATE
                                , ::DDS::ANY_VIEW_STATE
                                , ::DDS::ANY_INSTANCE_STATE );

          
        check_read_status(status, data1, 1, "t3 read2");

        if (data1[0].count != 1)
        {
            // test to see the accessing the "lost" (because of history.depth)
            // but still held by zero-copy sequence value works.
            ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) t3 ERROR: unexpected value for data1-pre.\n") ));
            test_failed = 1;

        }

        foo.key  = 1;
        foo.count = 2;

        // since depth=1 the previous sample will be "lost"
        // from the instance container.
        fast_dw->write(foo,
                        handle);

        // wait for write to propogate
        if (!wait_for_data(sub.in (), 5))
            ACE_ERROR_RETURN ((LM_ERROR,
                            ACE_TEXT("(%P|%t) t3 ERROR: timeout waiting for data.\n")),
                            1);

        SimpleZCSeq                  data2 (0, max_samples);
        ::TAO::DCPS::SampleInfoZCSeq info2 (0,max_samples);

        status = fast_dr->read(  data2 
                                , info2
                                , max_samples
                                , ::DDS::ANY_SAMPLE_STATE
                                , ::DDS::ANY_VIEW_STATE
                                , ::DDS::ANY_INSTANCE_STATE );

          
        check_read_status(status, data2, 1, "t3 read2");

        if (data1[0].count != 1)
        {
            // test to see the accessing the "lost" (because of history.depth)
            // but still held by zero-copy sequence value works.
            ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) t3 ERROR: unexpected value for data1-post.\n") ));
            test_failed = 1;

        }

        if (data2[0].count != 2)
        {
            ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) t3 ERROR: unexpected value for data2.\n") ));
            test_failed = 1;

        }
        status = fast_dr->return_loan(  data2 
                                        , info2 );

        check_return_loan_status(status, data2, 0, 0, "t3 return_loan2");

        // This return_loan will free the memory because the sample
        // has already been "lost" from the instance container.

        // 4/24/07 Note: breakpoint in the ls sequence destructor 
        // (part of Simple sample type) showed it was called when this 
        // block went out of scope. Good!
        // Note: the info sequence data is not destroyed/freed until
        //       the info1 object goes out of scope -- so it can 
        //       be reused without alloc & free.
        status = fast_dr->return_loan(  data1 
                                        , info1 );

        check_return_loan_status(status, data1, 0, 0, "t3 return_loan1");

      } // t3
      {
        //=====================================================
        // 4) show that the default is zero-copy read
        //    and automatic loan_return works.
        //=====================================================
        ACE_DEBUG((LM_INFO,"==== TEST 4 : show that the default is zero-copy read\n"));
        const CORBA::Long max_samples = 2;
        SimpleZCSeq                  data1;
        ::TAO::DCPS::SampleInfoZCSeq info1;

          
        DDS::ReturnCode_t status  ;
        status = fast_dr->read(  data1 
                                , info1
                                , max_samples
                                , ::DDS::ANY_SAMPLE_STATE
                                , ::DDS::ANY_VIEW_STATE
                                , ::DDS::ANY_INSTANCE_STATE );

          
        check_read_status(status, data1, 1, "t4 read2");

        // this should change the value returned by the next read
        data1[0].count = 999;

        {
            SimpleZCSeq                  data2;
            ::TAO::DCPS::SampleInfoZCSeq info2;
            status = fast_dr->read(  data2 
                                    , info2
                                    , max_samples
                                    , ::DDS::ANY_SAMPLE_STATE
                                    , ::DDS::ANY_VIEW_STATE
                                    , ::DDS::ANY_INSTANCE_STATE );

              
            check_read_status(status, data2, 1, "t4 read2");

            if (data1[0].count != data2[0].count)
            {
                ACE_ERROR ((LM_ERROR,
                        ACE_TEXT("(%P|%t) t4 ERROR: zero-copy failed.\n") ));
                test_failed = 1;

            }

            item = data2.getPtr(0);
            if (item->ref_count_ != 3)
            {
                ACE_ERROR ((LM_ERROR,
                        ACE_TEXT("(%P|%t) t4 ERROR: bad ref count %d expecting 3\n"), item->ref_count_ ));
                test_failed = 1;
            }

        } // data2 goes out of scope here and automatically return_loan'd 
            if (item->ref_count_ != 2)
            {
                ACE_ERROR ((LM_ERROR,
                        ACE_TEXT("(%P|%t) t4 ERROR: bad ref count %d expecting 2\n"), item->ref_count_ ));
                test_failed = 1;
            }
      } // t4
      // just the instance container should have a reference.
        if (item->ref_count_ != 1)
        {
            ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) t4 ERROR: bad ref count %d expecting 1\n"), item->ref_count_ ));
            test_failed = 1;
        }

      {
        //=====================================================
        // 5) show that return_loan and then read with same sequence works.
        //=====================================================
        ACE_DEBUG((LM_INFO,"==== TEST 5 : show that return_loan and then read with same sequence works.\n"));

        const CORBA::Long max_samples = 2;
        // 0 means zero-copy
        SimpleZCSeq                  data1 (0, max_samples);
        ::TAO::DCPS::SampleInfoZCSeq info1 (0,max_samples);
         
        foo.key  = 1;
        foo.count = 1;

        // since depth=1 the previous sample will be "lost"
        // from the instance container.
        fast_dw->write(foo, handle);

        // wait for write to propogate
        if (!wait_for_data(sub.in (), 5))
            ACE_ERROR_RETURN ((LM_ERROR,
                            ACE_TEXT("(%P|%t) t5 ERROR: timeout waiting for data.\n")),
                            1);

        DDS::ReturnCode_t status  ;
        status = fast_dr->read(  data1 
                                , info1
                                , max_samples
                                , ::DDS::ANY_SAMPLE_STATE
                                , ::DDS::ANY_VIEW_STATE
                                , ::DDS::ANY_INSTANCE_STATE );

          
        check_read_status(status, data1, 1, "t5 read2");

        if (data1[0].count != 1)
        {
            // test to see the accessing the "lost" (because of history.depth)
            // but still held by zero-copy sequence value works.
            ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) t5 ERROR: unexpected value for data1-pre.\n") ));
            test_failed = 1;

        }

        foo.key  = 1;
        foo.count = 2;

        // since depth=1 the previous sample will be "lost"
        // from the instance container.
        fast_dw->write(foo, handle);

        // wait for write to propogate
        if (!wait_for_data(sub.in (), 5))
            ACE_ERROR_RETURN ((LM_ERROR,
                            ACE_TEXT("(%P|%t) t5 ERROR: timeout waiting for data.\n")),
                            1);

        status = fast_dr->return_loan(  data1 
                                        , info1 );

        check_return_loan_status(status, data1, 0, 0, "t5 return_loan1");

        status = fast_dr->read(  data1 
                                , info1
                                , max_samples
                                , ::DDS::ANY_SAMPLE_STATE
                                , ::DDS::ANY_VIEW_STATE
                                , ::DDS::ANY_INSTANCE_STATE );

          
        check_read_status(status, data1, 1, "t5 read2");

        if (data1[0].count != 2)
        {
            ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) t5 ERROR: unexpected value for data2.\n") ));
            test_failed = 1;

        }

        status = fast_dr->return_loan(  data1 
                                        , info1 );

        check_return_loan_status(status, data1, 0, 0, "t5 return_loan1");

      } // t5
      {
        //=====================================================
        // 6) show that take takes 
        //=====================================================
        ACE_DEBUG((LM_INFO,"==== TEST 6 : show that take takes.\n"));

        const CORBA::Long max_samples = 2;
        // 0 means zero-copy
        SimpleZCSeq                  data1 (0, max_samples);
        ::TAO::DCPS::SampleInfoZCSeq info1 (0,max_samples);
         
        foo.key  = 1;
        foo.count = 1;

        // since depth=1 the previous sample will be "lost"
        // from the instance container.
        fast_dw->write(foo, handle);

        // wait for write to propogate
        if (!wait_for_data(sub.in (), 5))
            ACE_ERROR_RETURN ((LM_ERROR,
                            ACE_TEXT("(%P|%t) t6 ERROR: timeout waiting for data.\n")),
                            1);

        DDS::ReturnCode_t status  ;
        status = fast_dr->take(  data1 
                                , info1
                                , max_samples
                                , ::DDS::ANY_SAMPLE_STATE
                                , ::DDS::ANY_VIEW_STATE
                                , ::DDS::ANY_INSTANCE_STATE );

          
        check_read_status(status, data1, 1, "t6 read2");

        if (data1[0].count != 1)
        {
            // test to see the accessing the "lost" (because of history.depth)
            // but still held by zero-copy sequence value works.
            ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) t6 ERROR: unexpected value for data1-pre.\n") ));
            test_failed = 1;

        }

        // 0 means zero-copy
        SimpleZCSeq                  data2 (0, max_samples);
        ::TAO::DCPS::SampleInfoZCSeq info2 (0,max_samples);
        status = fast_dr->take(  data2 
                                , info2
                                , max_samples
                                , ::DDS::ANY_SAMPLE_STATE
                                , ::DDS::ANY_VIEW_STATE
                                , ::DDS::ANY_INSTANCE_STATE );

          
        if (status != ::DDS::RETCODE_NO_DATA)
        {
            ACE_ERROR ((LM_ERROR,
            ACE_TEXT("(%P|%t) %s ERROR: expected NO_DATA status from take!\n"), 
            "t6"));
            test_failed = 1;

        }

        // ?OK to return an empty loan?
        status = fast_dr->return_loan(  data2 
                                      , info2 );

        check_return_loan_status(status, data2, 0, 0, "t6 return_loan2");

        status = fast_dr->return_loan(  data1 
                                      , info1 );

        check_return_loan_status(status, data1, 0, 0, "t6 return_loan1");

      } // t6

      {
        //=====================================================
        // 7) show we can zero-copy read and then take and changes view state
        //=====================================================
        ACE_DEBUG((LM_INFO,"==== TEST 7 : show we can zero-copy read and then take and changes view state.\n"));

        const CORBA::Long max_samples = 2;
        // 0 means zero-copy
        SimpleZCSeq                  data1 (0, max_samples);
        ::TAO::DCPS::SampleInfoZCSeq info1 (0,max_samples);
         
        // It is important that the last test case took all of the samples!

        foo.key  = 7; // a new instance - this is important to get the correct view_state.
        foo.count = 7;

        // since depth=1 the previous sample will be "lost"
        // from the instance container.
        fast_dw->write(foo, handle);

        // wait for write to propogate
        if (!wait_for_data(sub.in (), 5))
            ACE_ERROR_RETURN ((LM_ERROR,
                            ACE_TEXT("(%P|%t) t7 ERROR: timeout waiting for data.\n")),
                            1);

        DDS::ReturnCode_t status  ;
        status = fast_dr->read(  data1 
                                , info1
                                , max_samples
                                , ::DDS::ANY_SAMPLE_STATE
                                , ::DDS::ANY_VIEW_STATE
                                , ::DDS::ANY_INSTANCE_STATE );

          
        check_read_status(status, data1, 1, "t7 read2");

        if (info1[0].view_state != ::DDS::NEW_VIEW_STATE)
        {
            ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) t7 ERROR: expected NEW view state.\n") ));
            test_failed = 1;
        }

        if (data1[0].count != 7)
        {
            // test to see the accessing the "lost" (because of history.depth)
            // but still held by zero-copy sequence value works.
            ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) t7 ERROR: unexpected value for data1-pre.\n") ));
            test_failed = 1;

        }

        // 0 means zero-copy
        SimpleZCSeq                  data2 (0, max_samples);
        ::TAO::DCPS::SampleInfoZCSeq info2 (0,max_samples);
        status = fast_dr->take(  data2 
                                , info2
                                , max_samples
                                , ::DDS::ANY_SAMPLE_STATE
                                , ::DDS::ANY_VIEW_STATE
                                , ::DDS::ANY_INSTANCE_STATE );

          
        check_read_status(status, data2, 1, "t7 take");
#if 0
    //wait for #10955 "DDS view_state implementation is wrong" to be fixed.
        if (info1[0].view_state != ::DDS::NOT_NEW_VIEW_STATE)
        {
            ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) t7 ERROR: expected NOT NEW view state.\n") ));
            test_failed = 1;
        }
#endif
        if (data1[0].count != data2[0].count)
        {
            ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) t7 ERROR: zero-copy read or take failed to provide same data.\n") ));
            test_failed = 1;

        }

        status = fast_dr->return_loan(  data2 
                                      , info2 );

        check_return_loan_status(status, data2, 0, 0, "t7 return_loan2");

        status = fast_dr->return_loan(  data1 
                                      , info1 );

        check_return_loan_status(status, data1, 0, 0, "t7 return_loan1");

      } // t7

      const CORBA::Long max_samples = 2;
      // scope must be larger than the sequences that use the allocator.
      BogusExampleAllocator<Test::Simple*,max_samples> the_allocator;
      {
        //=====================================================
        // 8) Show that an allocator can be provided.
        //=====================================================
        ACE_DEBUG((LM_INFO,"==== TEST 8 : Show that an allocator can be provided.\n"));

        const CORBA::Long max_samples = 2;
        // Note: the default allocator for a ZCSeq is very fast because
        // it will allocate from a pool on the stack and thus avoid
        // a heap allocation.
        // Note: because of the read/take preconditions the sequence does not need to resize.
        SimpleZCSeq                  data1 (0, max_samples, &the_allocator);
        ::TAO::DCPS::SampleInfoZCSeq info1 (0,max_samples);
         
        foo.key  = 1; 
        foo.count = 8;

        // since depth=1 the previous sample will be "lost"
        // from the instance container.
        fast_dw->write(foo, handle);

        // wait for write to propogate
        if (!wait_for_data(sub.in (), 5))
            ACE_ERROR_RETURN ((LM_ERROR,
                            ACE_TEXT("(%P|%t) t8 ERROR: timeout waiting for data.\n")),
                            1);

        DDS::ReturnCode_t status  ;
        status = fast_dr->read(  data1 
                                , info1
                                , max_samples
                                , ::DDS::ANY_SAMPLE_STATE
                                , ::DDS::ANY_VIEW_STATE
                                , ::DDS::ANY_INSTANCE_STATE );

          
        check_read_status(status, data1, 1, "t8 read2");

        if (1 != the_allocator.num_allocs())
        {
            ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) t8 ERROR: the allocator was not used.\n") ));
            test_failed = 1;
        }

        if (data1[0].count != 8)
        {
            // test to see the accessing the "lost" (because of history.depth)
            // but still held by zero-copy sequence value works.
            ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) t8 ERROR: unexpected value for data1-pre.\n") ));
            test_failed = 1;

        }

        // 0 means zero-copy
        SimpleZCSeq                  data2 (0, max_samples, &the_allocator);
        ::TAO::DCPS::SampleInfoZCSeq info2 (0,max_samples);
        status = fast_dr->take(  data2 
                                , info2
                                , max_samples
                                , ::DDS::ANY_SAMPLE_STATE
                                , ::DDS::ANY_VIEW_STATE
                                , ::DDS::ANY_INSTANCE_STATE );

          
        check_read_status(status, data2, 1, "t8 take");

        if (2 != the_allocator.num_allocs())
        {
            ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) t8 ERROR: the allocator was not used.\n") ));
            test_failed = 1;
        }

        if (data1[0].count != data2[0].count)
        {
            ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) t8 ERROR: zero-copy read or take failed to provide same data.\n") ));
            test_failed = 1;

        }

        status = fast_dr->return_loan(  data2 
                                      , info2 );

        // Note: the samples are freed in return_loan (if not reference by something else)
        //       but the sequence of pointers is not freed until data2 goes out of scope.

        check_return_loan_status(status, data2, 0, 0, "t8 return_loan2");


        status = fast_dr->return_loan(  data1 
                                      , info1 );

        check_return_loan_status(status, data1, 0, 0, "t8 return_loan1");


      } // Note: the sequence memory (pointers to samples) is freed here
        //       so the same sequence can be used over and over without
        //       allocating and freeing the array of pointers.
      if (2 != the_allocator.num_frees())
      {
          ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) t8 ERROR: the allocator was not used to free.\n") ));
          test_failed = 1;
      }

      {
        //=====================================================
        // 9) Show that loans are checked by delete_datareader.
        //=====================================================
      // !!!! note - this test should be the last because it deletes the datareader
        ACE_DEBUG((LM_INFO,"==== TEST 9 : Show that loans are checked by delete_datareader.\n"));

        const CORBA::Long max_samples = 2;
        SimpleZCSeq                  data1 (0, max_samples);
        ::TAO::DCPS::SampleInfoZCSeq info1 (0,max_samples);
         
        foo.key  = 1; 
        foo.count = 9;

        // since depth=1 the previous sample will be "lost"
        // from the instance container.
        fast_dw->write(foo, handle);

        // wait for write to propogate
        if (!wait_for_data(sub.in (), 5))
            ACE_ERROR_RETURN ((LM_ERROR,
                            ACE_TEXT("(%P|%t) t9 ERROR: timeout waiting for data.\n")),
                            1);

        DDS::ReturnCode_t status  ;
        status = fast_dr->read(  data1 
                                , info1
                                , max_samples
                                , ::DDS::ANY_SAMPLE_STATE
                                , ::DDS::ANY_VIEW_STATE
                                , ::DDS::ANY_INSTANCE_STATE );

          
        check_read_status(status, data1, 1, "t9 read2");

        if (data1[0].count != 9)
        {
            ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) t9 ERROR: unexpected value for data1-pre.\n") ));
            test_failed = 1;
        }

        status =   sub->delete_datareader(dr.in ());

        if (status != ::DDS::RETCODE_PRECONDITION_NOT_MET)
        {
            ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) t9 ERROR: delete_datawrite should have returned PRECONDITION_NOT_MET but returned retcode %d.\n"), status ));
            test_failed = 1;
        }

        status = fast_dr->return_loan(  data1 
                                      , info1 );

        check_return_loan_status(status, data1, 0, 0, "t9 return_loan");

        status =   sub->delete_datareader(dr.in ());

        if (status != ::DDS::RETCODE_OK)
        {
            ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) t9 ERROR: delete_datawrite failed with retcode %d.\n"), status ));
            test_failed = 1;
        }


      }
    }
  catch (const TestException&)
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) TestException caught in main.cpp. ")));
      return 1;
    }
      //======== clean up ============
      // Clean up publisher objects
//      pub->delete_contained_entities() ;

      pub->delete_datawriter(dw.in ());
      dp->delete_publisher(pub.in ());


      //clean up subscriber objects
//      sub->delete_contained_entities() ;

      //done above - in the tests sub->delete_datareader(dr.in ());
      dp->delete_subscriber(sub.in ());

      // clean up common objects
      dp->delete_topic(topic.in ());
      dpf->delete_participant(dp.in ());

      TheTransportFactory->release();
      TheServiceParticipant->shutdown ();

    }
  catch (const TestException&)
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) TestException caught in main.cpp. ")));
      return 1;
    }
  catch (const CORBA::Exception& ex)
    {
      ex._tao_print_exception ("Exception caught in main.cpp:");
      return 1;
    }

  // Note: The TransportImpl reference SHOULD be deleted before exit from
  //       main if the concrete transport libraries are loaded dynamically.
  //       Otherwise cleanup after main() will encount access vilation.
  reader_transport_impl = 0;
  writer_transport_impl = 0;
  return test_failed;
}
