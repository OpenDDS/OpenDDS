// -*- C++ -*-
// ============================================================================
/**
 *  @file   main.cpp
 *
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
#include "dds/DCPS/SafetyProfileStreams.h"

#include "dds/DCPS/StaticIncludes.h"

#include "ace/Arg_Shifter.h"
#include "ace/OS_NS_unistd.h"

#include <string.h>

using OpenDDS::DCPS::retcode_to_string;

class TestException
{
  public:

    TestException()  {}
    ~TestException() {}
};


const long  MY_DOMAIN   = 111;
const char* MY_TOPIC    = "foo";
const char* MY_TYPE     = "foo";

const ACE_Time_Value max_blocking_time(::DDS::DURATION_INFINITE_SEC);

int max_samples_per_instance = ::DDS::LENGTH_UNLIMITED;
int history_depth = 1 ;
bool support_client_side_BIT = false;
bool do_by_instance = false;

int test_failed = 0;

OpenDDS::DCPS::TransportImpl_rch reader_transport_impl;
OpenDDS::DCPS::TransportImpl_rch writer_transport_impl;

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


int wait_for_data (::DDS::Subscriber_ptr sub,
                   int timeout_sec)
{
  const int factor = 10;
  ACE_Time_Value small_time(0,1000000/factor);
  int timeout_loops = timeout_sec * factor;

  ::DDS::DataReaderSeq_var discard = new ::DDS::DataReaderSeq(10);
  while (timeout_loops-- > 0)
    {
      sub->get_datareaders (
                    discard.inout (),
                    ::DDS::NOT_READ_SAMPLE_STATE,
                    ::DDS::ANY_VIEW_STATE,
                    ::DDS::ANY_INSTANCE_STATE );
      if (discard->length () > 0)
        return 1;

      ACE_OS::sleep (small_time);
    }
  return 0;
}

/// parse the command line arguments
int parse_args (int argc, ACE_TCHAR *argv[])
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

    const ACE_TCHAR *currentArg = 0;

    if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-n"))) != 0)
    {
      max_samples_per_instance = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-d"))) != 0)
    {
      history_depth = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-z")) == 0)
    {
      TURN_ON_VERBOSE_DEBUG;
      arg_shifter.consume_arg();
    }
    else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-b")) == 0)
    {
      support_client_side_BIT = true;
      arg_shifter.consume_arg();
    }
    else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-i")) == 0)
    {
      do_by_instance = true;
      arg_shifter.consume_arg();
    }
    else
    {
      arg_shifter.ignore_arg ();
    }
  }
  // Indicates successful parsing of the command line
  return 0;
}

void check_read_status(DDS::ReturnCode_t status,
                       const Test::SimpleSeq& data,
                       CORBA::ULong expected,
                       const char* where)
{

      if (status == ::DDS::RETCODE_OK)
      {
          if (data.length() != expected)
          {
              ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) %C ERROR: expected %d samples but got %d\n"),
                  where, expected, data.length() ));
              test_failed = 1;
              throw TestException();
          }

      }
      else if (status == ::DDS::RETCODE_NO_DATA)
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT("(%P|%t) %C ERROR: reader received NO_DATA!\n"),
          where));
        test_failed = 1;
        throw TestException();
      }
      else if (status == ::DDS::RETCODE_PRECONDITION_NOT_MET)
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT("(%P|%t) %C ERROR: reader received PRECONDITION_NOT_MET!\n"),
          where));
        test_failed = 1;
        throw TestException();
      }
      else
      {
        ACE_ERROR((LM_ERROR,
            ACE_TEXT("(%P|%t) %C ERROR: unexpected status %d!\n"),
            where, status ));
        test_failed = 1;
        throw TestException();
      }
}


void check_return_loan_status(DDS::ReturnCode_t status,
                              const Test::SimpleSeq& data,
                              CORBA::ULong expected_len,
                              CORBA::ULong expected_max,
                              const char* where)
{

      if (status == ::DDS::RETCODE_OK)
      {
          if (data.length() != expected_len)
          {
              ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) %C ERROR: expected %d len but got %d\n"),
                  where, expected_len, data.length() ));
              test_failed = 1;
              throw TestException();
          }
          if (data.maximum() != expected_max)
          {
              ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) %C ERROR: expected %d maximum but got %d\n"),
                  where, expected_max, data.maximum() ));
              test_failed = 1;
              throw TestException();
          }

      }
      else if (status == ::DDS::RETCODE_NO_DATA)
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT("(%P|%t) %C ERROR: reader received NO_DATA!\n"),
          where));
        test_failed = 1;
        throw TestException();
      }
      else if (status == ::DDS::RETCODE_PRECONDITION_NOT_MET)
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT("(%P|%t) %C ERROR: reader received PRECONDITION_NOT_MET!\n"),
          where));
        test_failed = 1;
        throw TestException();
      }
      else
      {
        ACE_ERROR((LM_ERROR,
            ACE_TEXT("(%P|%t) %C ERROR: unexpected status %d!\n"),
            where, status ));
        test_failed = 1;
        throw TestException();
      }
}

int ACE_TMAIN (int argc, ACE_TCHAR *argv[])
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

      Test::SimpleTypeSupport_var fts = new Test::SimpleTypeSupportImpl();

      { //xxx dp scope
      ::DDS::DomainParticipant_var dp =
        dpf->create_participant(MY_DOMAIN,
                                PARTICIPANT_QOS_DEFAULT,
                                ::DDS::DomainParticipantListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      //xxx obj rc = 3
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
                          ::DDS::TopicListener::_nil(),
                          ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
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
                             ::DDS::SubscriberListener::_nil(),
                             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (sub.in ()))
      {
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT("(%P|%t) create_subscriber failed.\n")),
                           1);
      }

      // Create the publisher
      ::DDS::Publisher_var pub =
        dp->create_publisher(PUBLISHER_QOS_DEFAULT,
                             ::DDS::PublisherListener::_nil(),
                             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (pub.in ()))
      {
        ACE_ERROR_RETURN ((LM_ERROR,
                          ACE_TEXT("(%P|%t) create_publisher failed.\n")),
                          1);
      }

      // Create the datawriter
      ::DDS::DataWriterQos dw_qos;
      pub->get_default_datawriter_qos (dw_qos);

      dw_qos.history.kind = ::DDS::KEEP_LAST_HISTORY_QOS;
      dw_qos.history.depth = history_depth  ;
      dw_qos.resource_limits.max_samples_per_instance =
            max_samples_per_instance ;

      ::DDS::DataWriter_var dw = pub->create_datawriter(topic.in (),
                                        dw_qos,
                                        ::DDS::DataWriterListener::_nil(),
                                        ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

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
                                 ::DDS::DataReaderListener::_nil(),
                                 ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil (dr.in ()))
        {
          ACE_ERROR_RETURN ((LM_ERROR,
            ACE_TEXT("(%P|%t) create_datareader failed.\n")),
            1);
        }

        Test::SimpleDataWriter_var foo_dw
           = Test::SimpleDataWriter::_narrow(dw.in ());
      if (CORBA::is_nil (foo_dw.in ()))
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT("(%P|%t) Test::SimpleDataWriter::_narrow failed.\n")));
        return 1; // failure
      }

      Test::SimpleDataReader_var foo_dr
        = Test::SimpleDataReader::_narrow(dr.in ());
      if (CORBA::is_nil (foo_dr.in ()))
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT("(%P|%t) Test::SimpleDataReader::_narrow failed.\n")));
        return 1; // failure
      }

      // wait for association establishement before writing.
      // -- replaced this sleep with the while loop below;
      //    waiting on the one association we expect.
      //  ACE_OS::sleep(5); //REMOVE if not needed
      ::DDS::InstanceHandleSeq handles;
      while (1)
      {
          foo_dw->get_matched_subscriptions(handles);
          if (handles.length() > 0)
              break;
          else
              ACE_OS::sleep(ACE_Time_Value(0,200000));
      }

      // =============== do the test ====


      ::DDS::SubscriptionMatchedStatus matched;

      if (foo_dr->get_subscription_matched_status (matched) != ::DDS::RETCODE_OK)
      {
        ACE_ERROR_RETURN ((LM_ERROR,
          ACE_TEXT ("ERROR: failed to get subscription matched status\n")),
          1);
      }

      ::DDS::InstanceHandle_t writer_instance_handle = ::DDS::HANDLE_NIL;
      ::DDS::InstanceHandle_t reader_instance_handle = ::DDS::HANDLE_NIL;

          if (matched.total_count != 1)
            ACE_ERROR_RETURN((LM_ERROR,
              "TEST ERROR: expected subscription_match"
              " with count 1 but got %d\n",
              matched.total_count),
              9);

    try { // the real testing.
      ::Test::Simple foo;
      ::Test::LongSeq ls;
      //::Test::Simple::_ls_seq ls;
      ls.length(1);
      ls[0] = 5;
      foo.key  = 1;
      foo.count = 1;
      foo.text = CORBA::string_dup("t1");
      foo.ls = ls;


      writer_instance_handle
          = foo_dw->register_instance(foo);

      foo_dw->write(foo,
                     writer_instance_handle);


      // wait for new data for upto 5 seconds
      if (!wait_for_data(sub.in (), 5))
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT("(%P|%t) ERROR: timeout waiting for data.\n")),
                           1);


      OpenDDS::DCPS::ReceivedDataElement *item;
      {
        //=====================================================
        // 1) show that zero-copy is zero-copy
        //=====================================================
        ACE_DEBUG((LM_INFO,"==== TEST 1 : show that zero-copy is zero-copy\n"));
        const CORBA::Long max_samples = 2;
        // 0 means zero-copy
        Test::SimpleSeq      data1 (0, max_samples);
        ::DDS::SampleInfoSeq info1 (max_samples, 0, 0);


        DDS::ReturnCode_t status  ;
        if (do_by_instance)
          {
            status = foo_dr->read ( data1
                                    , info1
                                    , max_samples
                                    , ::DDS::ANY_SAMPLE_STATE
                                    , ::DDS::ANY_VIEW_STATE
                                    , ::DDS::ANY_INSTANCE_STATE );
            // remember this for later.
            reader_instance_handle = info1[0].instance_handle;
          }
        else
          {
            status = foo_dr->read ( data1
                                    , info1
                                    , max_samples
                                    , ::DDS::ANY_SAMPLE_STATE
                                    , ::DDS::ANY_VIEW_STATE
                                    , ::DDS::ANY_INSTANCE_STATE );
          }

        check_read_status(status, data1, 1, "t1 read2");

        // this should change the value returned by the next read
        data1[0].count = 999;

        Test::SimpleSeq      data2 (0, max_samples);
        ::DDS::SampleInfoSeq info2;
        if (do_by_instance)
          {
            status = foo_dr->read_instance(  data2
                                    , info2
                                    , max_samples
                                    , reader_instance_handle
                                    , ::DDS::ANY_SAMPLE_STATE
                                    , ::DDS::ANY_VIEW_STATE
                                    , ::DDS::ANY_INSTANCE_STATE );
          }
        else
          {
            status = foo_dr->read(  data2
                                    , info2
                                    , max_samples
                                    , ::DDS::ANY_SAMPLE_STATE
                                    , ::DDS::ANY_VIEW_STATE
                                    , ::DDS::ANY_INSTANCE_STATE );
          }


        check_read_status(status, data2, 1, "t1 read2");

        if (data1[0].count != data2[0].count)
        {
            ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) t1 ERROR: zero-copy failed.\n") ));
            test_failed = 1;

        }

        Test::SimpleSeq::PrivateMemberAccess data2_p(data2);
        item = data2_p.get_ptr(0);

        // test the assignment operator.
        Test::SimpleSeq      copy;
        ::DDS::SampleInfoSeq copyInfo;
        copy     = data2;
        copyInfo = info2;
        if (   copy.length() != data2.length()
            || copy[0].count != data2[0].count
            || item->ref_count() != 4)
          {
            ACE_ERROR ((LM_ERROR,
                ACE_TEXT("(%P|%t) t1 ERROR: assignment operator failed\n") ));
            test_failed = 1;
          }

        status = foo_dr->return_loan(copy, copyInfo );

        check_return_loan_status(status, copy, 0, 0, "t1 return_loan copy");

        if (item->ref_count() != 3)
        {
            ACE_ERROR ((LM_ERROR,
                ACE_TEXT("(%P|%t) t1 ERROR: bad ref count %d expecting 3\n"), item->ref_count() ));
            test_failed = 1;
        }


        status = foo_dr->return_loan(data2, info2 );

        check_return_loan_status(status, data2, 0, 0, "t1 return_loan2");

        if (item->ref_count() != 2)
        {
            ACE_ERROR ((LM_ERROR,
                ACE_TEXT("(%P|%t) t4 ERROR: bad ref count %d expecting 2\n"), item->ref_count() ));
            test_failed = 1;
        }
        status = foo_dr->return_loan(data1, info1 );

        check_return_loan_status(status, data1, 0, 0, "t1 return_loan1");

        // just the instance container should have a reference.
        if (item->ref_count() != 1)
        {
            ACE_ERROR ((LM_ERROR,
                ACE_TEXT("(%P|%t) t4 ERROR: bad ref count %d expecting 1\n"), item->ref_count() ));
            test_failed = 1;
        }
      } // t1

      {
        //=====================================================
        // 2) show that single-copy makes copies
        //=====================================================
        ACE_DEBUG((LM_INFO,"==== TEST 2 : show that single-copy makes copies\n"));

        const CORBA::Long max_samples = 2;
        // types supporting zero-copy read
        Test::SimpleSeq      data1 (max_samples);
        ::DDS::SampleInfoSeq info1 (max_samples);

        DDS::ReturnCode_t status  ;
        if (do_by_instance)
          {
            status = foo_dr->read_instance(  data1
                                    , info1
                                    , max_samples
                                    , reader_instance_handle
                                    , ::DDS::ANY_SAMPLE_STATE
                                    , ::DDS::ANY_VIEW_STATE
                                    , ::DDS::ANY_INSTANCE_STATE );
          }
        else
          {
            status = foo_dr->read(  data1
                                    , info1
                                    , max_samples
                                    , ::DDS::ANY_SAMPLE_STATE
                                    , ::DDS::ANY_VIEW_STATE
                                    , ::DDS::ANY_INSTANCE_STATE );
          }

        check_read_status(status, data1, 1, "t1 read2");

        // this should change the value returned by the next read
        data1[0].count = 888;
        data1[0].text = CORBA::string_dup("t2");

        Test::SimpleSeq      data2 (max_samples);
        ::DDS::SampleInfoSeq info2 (max_samples);
        if (do_by_instance)
          {
            status = foo_dr->read_instance ( data2
                                    , info2
                                    , max_samples
                                    , reader_instance_handle
                                    , ::DDS::ANY_SAMPLE_STATE
                                    , ::DDS::ANY_VIEW_STATE
                                    , ::DDS::ANY_INSTANCE_STATE );
          }
        else
          {
            status = foo_dr->read(  data2
                                    , info2
                                    , max_samples
                                    , ::DDS::ANY_SAMPLE_STATE
                                    , ::DDS::ANY_VIEW_STATE
                                    , ::DDS::ANY_INSTANCE_STATE );
          }

        check_read_status(status, data2, 1, "t2 read2");

        if (data1[0].count == data2[0].count)
        {
            ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) t2 ERROR: single-copy test failed for scalar.\n") ));
            test_failed = 1;

        }

        //ACE_DEBUG((LM_DEBUG,"%C != %C\n", data1[0].text.in(), data2[0].text.in() ));

        if (0 == strcmp(data1[0].text.in(), data2[0].text.in()))
        {
            ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) t2 ERROR: single-copy test failed for string.\n") ));
            test_failed = 1;

        }

        // test the assignment operator.
        Test::SimpleSeq      copy (max_samples+1);
        ::DDS::SampleInfoSeq copyInfo (max_samples+1);
        copy     = data2;
        copyInfo = info2;
        if (   copy.length() != data2.length()
            || copy[0].count != data2[0].count )
          {
            ACE_ERROR ((LM_ERROR,
                ACE_TEXT("(%P|%t) t2 ERROR: assignment operator failed\n") ));
            test_failed = 1;
          }

        status = foo_dr->return_loan(copy, copyInfo );

        check_return_loan_status(status, copy, 1, max_samples, "t2 return_loan copy");

        status = foo_dr->return_loan(  data2
                                      , info2 );

        check_return_loan_status(status, data2, 1, max_samples, "t2 return_loan2");

        status = foo_dr->return_loan(  data1
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
        ACE_DEBUG((LM_INFO,"==== TEST 3 : show that zero-copy reference counting works\n"));

        const CORBA::Long max_samples = 2;
        // 0 means zero-copy
        Test::SimpleSeq      data1 (0, max_samples);
        ::DDS::SampleInfoSeq info1;


        foo.key  = 1;
        foo.count = 1;

        // since depth=1 the previous sample will be "lost"
        // from the instance container.
        foo_dw->write(foo,
                        writer_instance_handle);

        // wait for write to propagate
        if (!wait_for_data(sub.in (), 5))
            ACE_ERROR_RETURN ((LM_ERROR,
                            ACE_TEXT("(%P|%t) t3 ERROR: timeout waiting for data.\n")),
                            1);

        DDS::ReturnCode_t status  ;
        if (do_by_instance)
          {
            status = foo_dr->read_instance(  data1
                                    , info1
                                    , max_samples
                                    , reader_instance_handle
                                    , ::DDS::ANY_SAMPLE_STATE
                                    , ::DDS::ANY_VIEW_STATE
                                    , ::DDS::ANY_INSTANCE_STATE );
          }
        else
          {
            status = foo_dr->read(  data1
                                    , info1
                                    , max_samples
                                    , ::DDS::ANY_SAMPLE_STATE
                                    , ::DDS::ANY_VIEW_STATE
                                    , ::DDS::ANY_INSTANCE_STATE );
          }

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
        foo_dw->write(foo,
                        writer_instance_handle);

        // wait for write to propagate
        if (!wait_for_data(sub.in (), 5))
            ACE_ERROR_RETURN ((LM_ERROR,
                            ACE_TEXT("(%P|%t) t3 ERROR: timeout waiting for data.\n")),
                            1);

        Test::SimpleSeq      data2 (0, max_samples);
        ::DDS::SampleInfoSeq info2;

        if (do_by_instance)
          {
            status = foo_dr->read_instance(  data2
                                    , info2
                                    , max_samples
                                    , reader_instance_handle
                                    , ::DDS::ANY_SAMPLE_STATE
                                    , ::DDS::ANY_VIEW_STATE
                                    , ::DDS::ANY_INSTANCE_STATE );
          }
        else
          {
            status = foo_dr->read(  data2
                                    , info2
                                    , max_samples
                                    , ::DDS::ANY_SAMPLE_STATE
                                    , ::DDS::ANY_VIEW_STATE
                                    , ::DDS::ANY_INSTANCE_STATE );
          }

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
        status = foo_dr->return_loan(  data2
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
        status = foo_dr->return_loan(  data1
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
        Test::SimpleSeq      data1;
        ::DDS::SampleInfoSeq info1;


        DDS::ReturnCode_t status  ;
        if (do_by_instance)
          {
            status = foo_dr->read_instance(  data1
                                    , info1
                                    , max_samples
                                    , reader_instance_handle
                                    , ::DDS::ANY_SAMPLE_STATE
                                    , ::DDS::ANY_VIEW_STATE
                                    , ::DDS::ANY_INSTANCE_STATE );
          }
        else
          {
            status = foo_dr->read(  data1
                                    , info1
                                    , max_samples
                                    , ::DDS::ANY_SAMPLE_STATE
                                    , ::DDS::ANY_VIEW_STATE
                                    , ::DDS::ANY_INSTANCE_STATE );
          }

        check_read_status(status, data1, 1, "t4 read2");

        // this should change the value returned by the next read
        data1[0].count = 999;

        {
            Test::SimpleSeq      data2;
            ::DDS::SampleInfoSeq info2;
        if (do_by_instance)
          {
            status = foo_dr->read_instance(  data2
                                    , info2
                                    , max_samples
                                    , reader_instance_handle
                                    , ::DDS::ANY_SAMPLE_STATE
                                    , ::DDS::ANY_VIEW_STATE
                                    , ::DDS::ANY_INSTANCE_STATE );
          }
        else
          {
            status = foo_dr->read(  data2
                                    , info2
                                    , max_samples
                                    , ::DDS::ANY_SAMPLE_STATE
                                    , ::DDS::ANY_VIEW_STATE
                                    , ::DDS::ANY_INSTANCE_STATE );
          }

            check_read_status(status, data2, 1, "t4 read2");

            if (data1[0].count != data2[0].count)
            {
                ACE_ERROR ((LM_ERROR,
                        ACE_TEXT("(%P|%t) t4 ERROR: zero-copy failed.\n") ));
                test_failed = 1;

            }

            Test::SimpleSeq::PrivateMemberAccess data2_p(data2);
            item = data2_p.get_ptr(0);
            if (item->ref_count() != 3)
            {
                ACE_ERROR ((LM_ERROR,
                        ACE_TEXT("(%P|%t) t4 ERROR: bad ref count %d expecting 3\n"), item->ref_count() ));
                test_failed = 1;
            }

        } // data2 goes out of scope here and automatically return_loan'd
            if (item->ref_count() != 2)
            {
                ACE_ERROR ((LM_ERROR,
                        ACE_TEXT("(%P|%t) t4 ERROR: bad ref count %d expecting 2\n"), item->ref_count() ));
                test_failed = 1;
            }
      } // t4
      // just the instance container should have a reference.
        if (item->ref_count() != 1)
        {
            ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) t4 ERROR: bad ref count %d expecting 1\n"), item->ref_count() ));
            test_failed = 1;
        }

      {
        //=====================================================
        // 5) show that return_loan and then read with same sequence works.
        //=====================================================
        ACE_DEBUG((LM_INFO,"==== TEST 5 : show that return_loan and then read with same sequence works.\n"));

        const CORBA::Long max_samples = 2;
        // 0 means zero-copy
        Test::SimpleSeq      data1 (0, max_samples);
        ::DDS::SampleInfoSeq info1;

        foo.key  = 1;
        foo.count = 1;

        // since depth=1 the previous sample will be "lost"
        // from the instance container.
        foo_dw->write(foo, writer_instance_handle);

        // wait for write to propagate
        if (!wait_for_data(sub.in (), 5))
            ACE_ERROR_RETURN ((LM_ERROR,
                            ACE_TEXT("(%P|%t) t5 ERROR: timeout waiting for data.\n")),
                            1);

        DDS::ReturnCode_t status  ;
        if (do_by_instance)
          {
            status = foo_dr->read_instance(  data1
                                    , info1
                                    , max_samples
                                    , reader_instance_handle
                                    , ::DDS::ANY_SAMPLE_STATE
                                    , ::DDS::ANY_VIEW_STATE
                                    , ::DDS::ANY_INSTANCE_STATE );
          }
        else
          {
            status = foo_dr->read(  data1
                                    , info1
                                    , max_samples
                                    , ::DDS::ANY_SAMPLE_STATE
                                    , ::DDS::ANY_VIEW_STATE
                                    , ::DDS::ANY_INSTANCE_STATE );
          }

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
        foo_dw->write(foo, writer_instance_handle);

        // wait for write to propagate
        if (!wait_for_data(sub.in (), 5))
            ACE_ERROR_RETURN ((LM_ERROR,
                            ACE_TEXT("(%P|%t) t5 ERROR: timeout waiting for data.\n")),
                            1);

        status = foo_dr->return_loan(  data1
                                        , info1 );

        check_return_loan_status(status, data1, 0, 0, "t5 return_loan1");

        if (do_by_instance)
          {
            status = foo_dr->read_instance(  data1
                                    , info1
                                    , max_samples
                                    , reader_instance_handle
                                    , ::DDS::ANY_SAMPLE_STATE
                                    , ::DDS::ANY_VIEW_STATE
                                    , ::DDS::ANY_INSTANCE_STATE );
          }
        else
          {
            status = foo_dr->read(  data1
                                    , info1
                                    , max_samples
                                    , ::DDS::ANY_SAMPLE_STATE
                                    , ::DDS::ANY_VIEW_STATE
                                    , ::DDS::ANY_INSTANCE_STATE );
          }

        check_read_status(status, data1, 1, "t5 read2");

        if (data1[0].count != 2)
        {
            ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) t5 ERROR: unexpected value for data2.\n") ));
            test_failed = 1;

        }

        status = foo_dr->return_loan(  data1
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
        Test::SimpleSeq     data1 (0, max_samples);
        ::DDS::SampleInfoSeq info1;

        foo.key  = 1;
        foo.count = 1;

        // since depth=1 the previous sample will be "lost"
        // from the instance container.
        foo_dw->write(foo, writer_instance_handle);

        // wait for write to propagate
        if (!wait_for_data(sub.in (), 5))
            ACE_ERROR_RETURN ((LM_ERROR,
                            ACE_TEXT("(%P|%t) t6 ERROR: timeout waiting for data.\n")),
                            1);

        DDS::ReturnCode_t status  ;
        if (do_by_instance)
          {
            status = foo_dr->take_instance(  data1
                                    , info1
                                    , max_samples
                                    , reader_instance_handle
                                    , ::DDS::ANY_SAMPLE_STATE
                                    , ::DDS::ANY_VIEW_STATE
                                    , ::DDS::ANY_INSTANCE_STATE );
          }
        else
          {
            status = foo_dr->take(  data1
                                    , info1
                                    , max_samples
                                    , ::DDS::ANY_SAMPLE_STATE
                                    , ::DDS::ANY_VIEW_STATE
                                    , ::DDS::ANY_INSTANCE_STATE );
          }

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
        Test::SimpleSeq     data2 (0, max_samples);
        ::DDS::SampleInfoSeq info2(max_samples, 0, 0); //testing alternate ctor
        if (do_by_instance)
          {
            status = foo_dr->take_instance ( data2
                                    , info2
                                    , max_samples
                                    , reader_instance_handle
                                    , ::DDS::ANY_SAMPLE_STATE
                                    , ::DDS::ANY_VIEW_STATE
                                    , ::DDS::ANY_INSTANCE_STATE );
          }
        else
          {
            status = foo_dr->take(  data2
                                    , info2
                                    , max_samples
                                    , ::DDS::ANY_SAMPLE_STATE
                                    , ::DDS::ANY_VIEW_STATE
                                    , ::DDS::ANY_INSTANCE_STATE );
          }


        if (status != ::DDS::RETCODE_NO_DATA)
        {
            ACE_ERROR ((LM_ERROR,
              ACE_TEXT("(%P|%t) %t6 ERROR: expected NO_DATA status from take!\n")));
            test_failed = 1;

        }

        // ?OK to return an empty loan?
        status = foo_dr->return_loan(  data2
                                      , info2 );

        check_return_loan_status(status, data2, 0, 0, "t6 return_loan2");

        status = foo_dr->return_loan(  data1
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
        Test::SimpleSeq     data1 (0, max_samples);
        ::DDS::SampleInfoSeq info1;

        // It is important that the last test case took all of the samples!

        foo.key  = 7; // a new instance - this is important to get the correct view_state.
        foo.count = 7;

        // since depth=1 the previous sample will be "lost"
        // from the instance container.
        foo_dw->write(foo,  ::DDS::HANDLE_NIL /*don't use instance_handle_ because it is a different instance */);

        // wait for write to propagate
        if (!wait_for_data(sub.in (), 5))
            ACE_ERROR_RETURN ((LM_ERROR,
                            ACE_TEXT("(%P|%t) t7 ERROR: timeout waiting for data.\n")),
                            1);

        DDS::ReturnCode_t status  ;
        status = foo_dr->read(  data1
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
        Test::SimpleSeq     data2 (0, max_samples);
        ::DDS::SampleInfoSeq info2;
        status = foo_dr->take(  data2
                                , info2
                                , max_samples
                                , ::DDS::ANY_SAMPLE_STATE
                                , ::DDS::ANY_VIEW_STATE
                                , ::DDS::ANY_INSTANCE_STATE );


        check_read_status(status, data2, 1, "t7 take");

        if (info2[0].view_state != ::DDS::NOT_NEW_VIEW_STATE)
        {
            ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) t7 ERROR: expected NOT NEW view state.\n") ));
            test_failed = 1;
        }

        if (data1[0].count != data2[0].count)
        {
            ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) t7 ERROR: zero-copy read or take failed to provide same data.\n") ));
            test_failed = 1;

        }

        status = foo_dr->return_loan(  data2
                                      , info2 );

        check_return_loan_status(status, data2, 0, 0, "t7 return_loan2");

        status = foo_dr->return_loan(  data1
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
        ACE_DEBUG((LM_INFO,"==== TEST 8 : show that an allocator can be provided.\n"));

        const CORBA::Long max_samples = 2;
        // Note: the default allocator for a ZCSeq is very fast because
        // it will allocate from a pool on the stack and thus avoid
        // a heap allocation.
        // Note: because of the read/take preconditions the sequence does not need to resize.
        Test::SimpleSeq     data1 (0, max_samples, &the_allocator);
        ::DDS::SampleInfoSeq info1;

        foo.key  = 1;
        foo.count = 8;

        // since depth=1 the previous sample will be "lost"
        // from the instance container.
        foo_dw->write(foo, writer_instance_handle);

        // wait for write to propagate
        if (!wait_for_data(sub.in (), 5))
            ACE_ERROR_RETURN ((LM_ERROR,
                            ACE_TEXT("(%P|%t) t8 ERROR: timeout waiting for data.\n")),
                            1);

        DDS::ReturnCode_t status  ;
        status = foo_dr->read(  data1
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
        Test::SimpleSeq     data2 (0, max_samples, &the_allocator);
        ::DDS::SampleInfoSeq info2;
        status = foo_dr->take(  data2
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

        status = foo_dr->return_loan(  data2
                                      , info2 );

        // Note: the samples are freed in return_loan (if not reference by something else)
        //       but the sequence of pointers is not freed until data2 goes out of scope.

        check_return_loan_status(status, data2, 0, 0, "t8 return_loan2");


        status = foo_dr->return_loan(  data1
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
        //==================================================================
        // 9) Show that the ZC sequence impl meets CORBA C++ mapping reqmts
        //==================================================================
        ACE_DEBUG((LM_INFO, ACE_TEXT("==== TEST 9 : show that the ZC sequence")
                   ACE_TEXT(" impl meets CORBA C++ mapping reqmts.\n")));
        using Test::SimpleSeq;
        using Test::Simple;
        SimpleSeq default_ctor;
        SimpleSeq copy_ctor(default_ctor);
        CORBA::ULong max(10);
        SimpleSeq max_ctor(max);
        copy_ctor = max_ctor; //test operator=
        if (copy_ctor.maximum() < max)
        {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(P%|%t) t9 ERROR: maximum() should return at least 10\n")));
          test_failed = 1;
        }
        Simple* buffer = SimpleSeq::allocbuf(max);
        SimpleSeq buf_ctor(max, 0, buffer);
        if (buf_ctor.release())
        {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(P%|%t) t9 ERROR: release() should be false\n")));
          test_failed = 1;
        }

        copy_ctor.replace(max, 0, buffer);
        Simple* buffer2 = copy_ctor.get_buffer();
        const SimpleSeq& const_cc = copy_ctor;
        const Simple* buffer3 = const_cc.get_buffer();
        if (buffer3 == 0)
        {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("(%P|%t) t9 ERROR: get_buffer() returned 0\n")));
          test_failed = 1;
        }
        SimpleSeq::freebuf(buffer2);
        {
          SimpleSeq orphanTest(const_cc);
          buffer2 = orphanTest.get_buffer(true);
          SimpleSeq::freebuf(buffer2);
        }

        max_ctor.length(max);
        for (CORBA::ULong i = 0; i < max; ++i)
        {
          max_ctor[i].key = 42 + i;
        }
        for (CORBA::ULong i = 0; i < max; ++i)
        {
          if (max_ctor[i].key != CORBA::Long(42 + i))
          {
            ACE_ERROR((LM_ERROR, ACE_TEXT("(P%|%t) t9 ERROR: Didn't get expected data out of the sequence.\n")));
            test_failed = 1;
          }
        }
        const SimpleSeq& const_seq = max_ctor;
        for (CORBA::ULong i = 0; i < max; ++i)
        {
          if (const_seq[i].key != CORBA::Long(42 + i))
          {
            ACE_ERROR((LM_ERROR, ACE_TEXT("(P%|%t) t9 ERROR: Didn't get expected data out of the sequence (const).\n")));
            test_failed = 1;
          }
        }

      }

      {
        //=====================================================
        // 10) Show that loans are checked by delete_datareader.
        //=====================================================
      // !!!! note - this test should be the last because it deletes the datareader
        ACE_DEBUG((LM_INFO,"==== TEST 10: show that loans are checked by delete_datareader.\n"));

        const CORBA::Long max_samples = 2;
        // Initialize the ZeroCopySeq and ZeroCopyInfoSeq objects for read
        // operation.
        Test::SimpleSeq     data0 (0, max_samples);
        ::DDS::SampleInfoSeq info0;

        foo.key  = 1;
        foo.count = 9;

        // since depth=1 the previous sample will be "lost"
        // from the instance container.
        foo_dw->write(foo, writer_instance_handle);

        // wait for write to propagate
        if (!wait_for_data(sub.in (), 5))
            ACE_ERROR_RETURN ((LM_ERROR,
                            ACE_TEXT("(%P|%t) t10 ERROR: timeout waiting for data.\n")),
                            1);

        DDS::ReturnCode_t status  ;
        status = foo_dr->read(  data0
                                , info0
                                , max_samples
                                , ::DDS::ANY_SAMPLE_STATE
                                , ::DDS::ANY_VIEW_STATE
                                , ::DDS::ANY_INSTANCE_STATE );

        // Copy read sample sequence and use it to check read status.
        Test::SimpleSeq     data1 (data0);
        ::DDS::SampleInfoSeq info1 (info0);

        check_read_status(status, data1, 1, "t10 read2");

        if (data1[0].count != 9)
        {
            ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) t10 ERROR: unexpected value for data1-pre.\n") ));
            test_failed = 1;
        }

        status = sub->delete_datareader(dr.in());
        if (status != ::DDS::RETCODE_PRECONDITION_NOT_MET) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) t10 ERROR: ")
            ACE_TEXT("expected PRECONDITION_NOT_MET from delete_datareader, ")
            ACE_TEXT("but it returned: %C\n"), retcode_to_string(status)));
          test_failed = 1;
        }

        // Return the "loan" of read samples to datareader.
        status = foo_dr->return_loan(  data0
                                      , info0 );

        check_return_loan_status(status, data0, 0, 0, "t10 return_loan");

        // Return the "loan" of copied samples to the datareader.
        status = foo_dr->return_loan(  data1
                                      , info1 );

        check_return_loan_status(status, data1, 0, 0, "t10 return_loan");

        status = sub->delete_datareader(dr.in ());
        if (status != ::DDS::RETCODE_OK) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) t10 ERROR: ")
            ACE_TEXT("delete_datareader returned: %C\n"),
            retcode_to_string(status)));
          test_failed = 1;
        }
      }
      {
        //=====================================================
        // 11) test length of sequence
        //=====================================================
        ACE_DEBUG((LM_INFO,"==== TEST 11: Set length on zero copy sequence.\n"));
        Test::SimpleSeq      seq;
        seq.length(7);
        if (seq.length() != 7) {
          ACE_ERROR ((LM_ERROR,
                      ACE_TEXT("(%P|%t) t11 ERROR: length %d when set to 7.\n"),
                      seq.length()));
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

      reader_transport_impl.reset();
      writer_transport_impl.reset();

      // clean up common objects
      dp->delete_topic(topic.in ());
      dpf->delete_participant(dp.in ());
//xx dp::Entity::Object::muxtex_refcount_ = 3

      }

      TheServiceParticipant->shutdown ();

    } //xxx dp::Entity::Object::muxtex_refcount_ = 1
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

  return test_failed;
}
