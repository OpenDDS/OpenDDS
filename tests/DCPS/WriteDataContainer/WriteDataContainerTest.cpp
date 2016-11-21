#include "ace/Condition_Recursive_Thread_Mutex.h"
#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DCPS/WriteDataContainer.h"
#include "dds/DCPS/Qos_Helper.h"
#include "ace/Arg_Shifter.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "SimpleTypeSupportImpl.h"
#include "dds/DCPS/InstanceHandle.h"
#include "dds/DCPS/DomainParticipantImpl.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/DataWriterImpl_T.h"

#include "../common/TestSupport.h"

#include "ace/Task.h"

#include <cstdlib>
#include <iostream>
#include <string>

using namespace std;
using namespace DDS;
using namespace OpenDDS::DCPS;

const long  MY_DOMAIN   = 411;
const char* MY_TOPIC    = "topic_foo";
const char* MY_TYPE     = "type_foo";

const ACE_Time_Value max_blocking_time(::DDS::DURATION_INFINITE_SEC);

int MAX_SAMPLES_PER_INSTANCE = 2;//::DDS::LENGTH_UNLIMITED;
int HISTORY_DEPTH = 2;
int MAX_SAMPLES = 3;
int MAX_BLOCKING_TIME_SEC = 1;
int MAX_BLOCKING_TIME_NANO = 0;

int test_failed = 0;

class TestException
{
  public:

    TestException()  {}
    ~TestException() {}
};

class TestParticipantImpl : public DomainParticipantImpl
{
public:
  TestParticipantImpl()
    : DomainParticipantImpl(0,
                            MY_DOMAIN,
                            GUID_UNKNOWN,
                            PARTICIPANT_QOS_DEFAULT,
                            ::DDS::DomainParticipantListener::_nil(),
                            ::OpenDDS::DCPS::DEFAULT_STATUS_MASK) {};

  virtual ~TestParticipantImpl() {};

  virtual DDS::InstanceHandle_t get_instance_handle() { return this->participant_handles_.next();}

private:
  InstanceHandleGenerator participant_handles_;
};

namespace Test
{
  typedef OpenDDS::DCPS::DataWriterImpl_T<Simple> SimpleDataWriterImpl;
}

class DDS_TEST
{
public:
  DDS_TEST()
    : delayed_deliver_container_(0)
    , element_to_deliver_(0)
  {};

  ~DDS_TEST() {};
  void substitute_dw_particpant (DataWriterImpl* dw, TestParticipantImpl* tp) { dw->participant_servant_ = tp;}

  ACE_Message_Block* dds_marshal(Test::SimpleDataWriterImpl* dw,
                                 const ::Test::Simple& instance_data,
                                 OpenDDS::DCPS::MarshalingType marshaling_type) {return dw->dds_marshal(instance_data, marshaling_type);}

  void prep_delayed_deliver(WriteDataContainer* wdc, DataSampleElement* element) {
    this->element_to_deliver_ = element;
    this->delayed_deliver_container_ = wdc;
  }


  void delayed_deliver () {
    ACE_GUARD (ACE_Recursive_Thread_Mutex,
        guard,
        this->lock_wdc(delayed_deliver_container_));

    this->delayed_deliver_container_->data_delivered(this->element_to_deliver_);
    this->log_send_state_lists("DDS_TEST data_delivered complete:", this->delayed_deliver_container_);

  }

  void log_send_state_lists (std::string description, WriteDataContainer* wdc) const {
    ACE_DEBUG((LM_DEBUG, "\n%T %C\n\tTest Data Container send state lists: unsent(%d), sending(%d), sent(%d), num_all_samples(%d), num_instances(%d)\n",
               description.c_str(),
               wdc->unsent_data_.size(),
               wdc->sending_data_.size(),
               wdc->sent_data_.size(),
               wdc->num_all_samples(),
               wdc->instances_.size()));
  }

  void log_perceived_qos_limits (WriteDataContainer* wdc) const {
    ACE_DEBUG((LM_DEBUG, "Test Data Container perceived qos limits: max_samples_per_instance(%d), max_num_instances(%d), max_num_samples(%d)\n",
               wdc->max_samples_per_instance_,
               wdc->max_num_instances_,
               wdc->max_num_samples_));
  }

  void log_dw_qos_limits (DataWriterQos& qos) const {
    ACE_DEBUG((LM_DEBUG, "DW qos limits: history_depth(%d) <= max_samples_per_instance(%d) <= max_samples(%d), max_instances(%d)\n",
               qos.history.depth,
               qos.resource_limits.max_samples_per_instance,
               qos.resource_limits.max_samples,
               qos.resource_limits.max_instances));
  }

  ACE_Recursive_Thread_Mutex& lock_wdc(WriteDataContainer* wdc) {return wdc->lock_;}

  WriteDataContainer* get_test_data_container(::DDS::DataWriterQos dw_qos, Test::SimpleDataWriterImpl* fast_dw)
  {
    const bool reliable = dw_qos.reliability.kind == DDS::RELIABLE_RELIABILITY_QOS;

              bool resource_blocking = false;

              size_t n_chunks = TheServiceParticipant->n_chunks();
              int depth = dw_qos.resource_limits.max_samples_per_instance;
              if (depth == DDS::LENGTH_UNLIMITED)
                depth = 0x7fffffff;

              if (dw_qos.resource_limits.max_samples != DDS::LENGTH_UNLIMITED) {
                n_chunks = dw_qos.resource_limits.max_samples;

                if (dw_qos.resource_limits.max_instances == DDS::LENGTH_UNLIMITED) {
                  resource_blocking = true;

                } else {
                  resource_blocking =
                    (dw_qos.resource_limits.max_samples < dw_qos.resource_limits.max_instances)
                    || (dw_qos.resource_limits.max_samples <
                        (dw_qos.resource_limits.max_instances * depth));
                }
              }
            #ifndef OPENDDS_NO_PERSISTENCE_PROFILE
              // Get data durability cache if DataWriter QoS requires durable
              // samples.  Publisher servant retains ownership of the cache.
              DataDurabilityCache* const durability_cache =
                TheServiceParticipant->get_data_durability_cache(dw_qos.durability);
            #endif

              CORBA::Long max_instances = 0;

              if (reliable && dw_qos.resource_limits.max_instances != DDS::LENGTH_UNLIMITED)
                max_instances = dw_qos.resource_limits.max_instances;

              CORBA::Long max_total_samples = 10;

              if (reliable && resource_blocking)
                max_total_samples = dw_qos.resource_limits.max_samples;

              return new WriteDataContainer(fast_dw,
                                            depth,
                                            depth,
                                            dw_qos.reliability.max_blocking_time,
                                            n_chunks,
                                            MY_DOMAIN,
                                            MY_TOPIC,
                                            MY_TYPE,
                                    #ifndef OPENDDS_NO_PERSISTENCE_PROFILE
                                            durability_cache,
                                            dw_qos.durability_service,
                                    #endif
                                            max_instances,
                                            max_total_samples);

  }

  void get_default_datawriter_qos(::DDS::DataWriterQos& initial_DataWriterQos_) {

    DDS::DurabilityQosPolicy initial_DurabilityQosPolicy;
    initial_DurabilityQosPolicy.kind = DDS::VOLATILE_DURABILITY_QOS;

    DDS::DurabilityServiceQosPolicy initial_DurabilityServiceQosPolicy;
    initial_DurabilityServiceQosPolicy.service_cleanup_delay.sec =
      DDS::DURATION_ZERO_SEC;
    initial_DurabilityServiceQosPolicy.service_cleanup_delay.nanosec =
      DDS::DURATION_ZERO_NSEC;
    initial_DurabilityServiceQosPolicy.history_kind =
      DDS::KEEP_LAST_HISTORY_QOS;
    initial_DurabilityServiceQosPolicy.history_depth = 1;
    initial_DurabilityServiceQosPolicy.max_samples =
      DDS::LENGTH_UNLIMITED;
    initial_DurabilityServiceQosPolicy.max_instances =
      DDS::LENGTH_UNLIMITED;
    initial_DurabilityServiceQosPolicy.max_samples_per_instance =
      DDS::LENGTH_UNLIMITED;

    DDS::DeadlineQosPolicy initial_DeadlineQosPolicy;
    initial_DeadlineQosPolicy.period.sec = DDS::DURATION_INFINITE_SEC;
    initial_DeadlineQosPolicy.period.nanosec = DDS::DURATION_INFINITE_NSEC;

    DDS::LatencyBudgetQosPolicy initial_LatencyBudgetQosPolicy;
    initial_LatencyBudgetQosPolicy.duration.sec = DDS::DURATION_ZERO_SEC;
    initial_LatencyBudgetQosPolicy.duration.nanosec = DDS::DURATION_ZERO_NSEC;

    DDS::LivelinessQosPolicy initial_LivelinessQosPolicy;
    initial_LivelinessQosPolicy.kind = DDS::AUTOMATIC_LIVELINESS_QOS;
    initial_LivelinessQosPolicy.lease_duration.sec = DDS::DURATION_INFINITE_SEC;
    initial_LivelinessQosPolicy.lease_duration.nanosec = DDS::DURATION_INFINITE_NSEC;

    DDS::ReliabilityQosPolicy initial_ReliabilityQosPolicy;
    initial_ReliabilityQosPolicy.kind = DDS::BEST_EFFORT_RELIABILITY_QOS;
    initial_ReliabilityQosPolicy.max_blocking_time.sec = DDS::DURATION_INFINITE_SEC;
    initial_ReliabilityQosPolicy.max_blocking_time.nanosec = DDS::DURATION_INFINITE_NSEC;

    DDS::DestinationOrderQosPolicy initial_DestinationOrderQosPolicy;
    initial_DestinationOrderQosPolicy.kind = DDS::BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS;

    DDS::HistoryQosPolicy initial_HistoryQosPolicy;
    initial_HistoryQosPolicy.kind = DDS::KEEP_LAST_HISTORY_QOS;
    initial_HistoryQosPolicy.depth = 1;

    DDS::ResourceLimitsQosPolicy initial_ResourceLimitsQosPolicy;
    initial_ResourceLimitsQosPolicy.max_samples = DDS::LENGTH_UNLIMITED;
    initial_ResourceLimitsQosPolicy.max_instances = DDS::LENGTH_UNLIMITED;
    initial_ResourceLimitsQosPolicy.max_samples_per_instance = DDS::LENGTH_UNLIMITED;

    DDS::TransportPriorityQosPolicy initial_TransportPriorityQosPolicy;
    initial_TransportPriorityQosPolicy.value = 0;

    DDS::LifespanQosPolicy initial_LifespanQosPolicy;
    initial_LifespanQosPolicy.duration.sec = DDS::DURATION_INFINITE_SEC;
    initial_LifespanQosPolicy.duration.nanosec = DDS::DURATION_INFINITE_NSEC;

    DDS::UserDataQosPolicy initial_UserDataQosPolicy;

    DDS::OwnershipQosPolicy initial_OwnershipQosPolicy;
    initial_OwnershipQosPolicy.kind = DDS::SHARED_OWNERSHIP_QOS;
    DDS::OwnershipStrengthQosPolicy initial_OwnershipStrengthQosPolicy;
#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
    initial_OwnershipStrengthQosPolicy.value = 0;
#else
    ACE_UNUSED_ARG(initial_OwnershipStrengthQosPolicy);
#endif
    DDS::WriterDataLifecycleQosPolicy   initial_WriterDataLifecycleQosPolicy;
    initial_WriterDataLifecycleQosPolicy.autodispose_unregistered_instances = true;

    initial_DataWriterQos_.durability = initial_DurabilityQosPolicy;
    initial_DataWriterQos_.durability_service = initial_DurabilityServiceQosPolicy;
    initial_DataWriterQos_.deadline = initial_DeadlineQosPolicy;
    initial_DataWriterQos_.latency_budget = initial_LatencyBudgetQosPolicy;
    initial_DataWriterQos_.liveliness = initial_LivelinessQosPolicy;
    initial_DataWriterQos_.reliability = initial_ReliabilityQosPolicy;
    initial_DataWriterQos_.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
    initial_DataWriterQos_.reliability.max_blocking_time.sec = 0;
    initial_DataWriterQos_.reliability.max_blocking_time.nanosec = 100000000;
    initial_DataWriterQos_.destination_order = initial_DestinationOrderQosPolicy;
    initial_DataWriterQos_.history = initial_HistoryQosPolicy;
    initial_DataWriterQos_.resource_limits = initial_ResourceLimitsQosPolicy;
    initial_DataWriterQos_.transport_priority = initial_TransportPriorityQosPolicy;
    initial_DataWriterQos_.lifespan = initial_LifespanQosPolicy;
    initial_DataWriterQos_.user_data = initial_UserDataQosPolicy;
    initial_DataWriterQos_.ownership = initial_OwnershipQosPolicy;
  #ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
    initial_DataWriterQos_.ownership_strength = initial_OwnershipStrengthQosPolicy;
  #endif
    initial_DataWriterQos_.writer_data_lifecycle = initial_WriterDataLifecycleQosPolicy;
  }

  WriteDataContainer* delayed_deliver_container_;
  DataSampleElement* element_to_deliver_;
};

// Writer thread.
class Delayed_Deliver_Handler : public ACE_Task_Base
{
public:
  DDS_TEST* test_;

  Delayed_Deliver_Handler (DDS_TEST* test) : test_(test)
  {}

  virtual int svc (void)
  {
    test_->delayed_deliver();
    return 0;
  }
};

/// parse the command line arguments
int parse_args(int argc, ACE_TCHAR *argv[])
{

  u_long mask = ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS);
  ACE_LOG_MSG->priority_mask(mask | LM_TRACE | LM_DEBUG, ACE_Log_Msg::PROCESS);
  ACE_Arg_Shifter arg_shifter(argc, argv);

  while (arg_shifter.is_anything_left())
  {
    // options:
    //  -n max_samples_per_instance defaults to INFINITE
    //  -d history.depth            defaults to 1
    //  -z                          verbose transport debug

    if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-z")) == 0)
    {
      TURN_ON_VERBOSE_DEBUG;
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

int run_test(int argc, ACE_TCHAR *argv[])
{
  u_long mask = ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS);
  ACE_LOG_MSG->priority_mask(mask | LM_TRACE | LM_DEBUG, ACE_Log_Msg::PROCESS);

  ACE_DEBUG((LM_DEBUG,"(%P|%t) write data container test start\n"));

  parse_args (argc, argv);

  try
    {
    TestParticipantImpl* tpi = new TestParticipantImpl();
    DDS_TEST* test = new DDS_TEST();
    ::DDS::DataWriterQos dw_qos;
    test->get_default_datawriter_qos(dw_qos);

      try { // the real testing.

        { //Test Case 1 scope
          ACE_DEBUG((LM_INFO,
                     ACE_TEXT("\n\n==== TEST case 1 : Reliable, Keep All, max_samples_per_instance = 2, max samples = 3.\n")
                     ACE_TEXT("Single instance: Should block on third obtain buffer due to max_samples_per_instance\n")
                     ACE_TEXT("===============================================\n")));

          dw_qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
          dw_qos.history.depth = ::DDS::LENGTH_UNLIMITED;

          dw_qos.resource_limits.max_samples_per_instance = MAX_SAMPLES_PER_INSTANCE;
          dw_qos.resource_limits.max_samples = MAX_SAMPLES;

          Test::SimpleDataWriterImpl* fast_dw = new Test::SimpleDataWriterImpl();
          test->substitute_dw_particpant(fast_dw, tpi);
          WriteDataContainer* test_data_container  = test->get_test_data_container(dw_qos, fast_dw);

          test->log_dw_qos_limits(dw_qos);
          test->log_perceived_qos_limits(test_data_container);
          test->log_send_state_lists("Initial Setup:", test_data_container);

          Test::Simple foo1;
          foo1.key  = 1;
          foo1.count = 1;

          ACE_Message_Block* mb = test->dds_marshal(fast_dw, foo1, OpenDDS::DCPS::KEY_ONLY_MARSHALING);

          ::DDS::InstanceHandle_t handle1 = DDS::HANDLE_NIL;

          DDS::ReturnCode_t retval = test_data_container->register_instance(handle1, mb);
          test->log_send_state_lists("After registering instance 1", test_data_container);

          if (retval != DDS::RETCODE_OK) {
            ACE_ERROR((LM_ERROR, "register instance failed\n"));
          }

          DDS::ReturnCode_t ret;

          //obtain the WriteDataContainer's lock as would be normal
          //when obtained by the datawriter during a write before accessing
          //the WriteDataContainer
          ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
              guard,
              test->lock_wdc(test_data_container),
              ::DDS::RETCODE_ERROR);

          DataSampleElement* element_0 = 0;
          ret = test_data_container->obtain_buffer(element_0, handle1);
          test->log_send_state_lists("After obtain buffer", test_data_container);

          if (ret != DDS::RETCODE_OK) {
            ACE_ERROR((LM_ERROR, "obtain buffer failed for element 0\n"));
          }

          element_0->set_sample(mb);

          if (ret != DDS::RETCODE_OK) {
            ACE_ERROR((LM_ERROR, "failed to write element 0\n"));
            return ret;
          }

          ret = test_data_container->enqueue(element_0, handle1);
          test->log_send_state_lists("After enqueue", test_data_container);

          SendStateDataSampleList temp;
          test_data_container->get_unsent_data(temp);
          test->log_send_state_lists("After get_unsent_data", test_data_container);

          if (ret != DDS::RETCODE_OK) {
            ACE_ERROR((LM_ERROR, "failed to enqueue element 0\n"));
          }
          ACE_Message_Block* mb1 = test->dds_marshal(fast_dw, foo1, OpenDDS::DCPS::KEY_ONLY_MARSHALING);

          DataSampleElement* element_1 = 0;
          ret = test_data_container->obtain_buffer(element_1, handle1);
          test->log_send_state_lists("After obtain buffer", test_data_container);

          if (ret != DDS::RETCODE_OK) {
            ACE_ERROR((LM_ERROR, "obtain buffer failed for element 1\n"));
          }

          element_1->set_sample(mb1);

          if (ret != DDS::RETCODE_OK) {
            return ret;
          }

          ret = test_data_container->enqueue(element_1, handle1);
          test->log_send_state_lists("After enqueue", test_data_container);

          temp.reset();
          test_data_container->get_unsent_data(temp);
          test->log_send_state_lists("After get_unsent_data", test_data_container);

          if (ret != DDS::RETCODE_OK) {
            ACE_ERROR((LM_ERROR, "failed to enqueue element 1\n"));
          }
          DataSampleElement* element_2 = 0;
          ret = test_data_container->obtain_buffer(element_2, handle1);
          test->log_send_state_lists("After obtain buffer which should block", test_data_container);

          TEST_ASSERT(errno == ETIME);

          test->log_send_state_lists("After TEST_ASSERT timeout", test_data_container);

          test_data_container->unregister_all();
          guard.release();
          delete test_data_container;
          delete fast_dw;
        } //End Test Case 1 scope

        { //Test Case 2 scope
          //=====================================================
          ACE_DEBUG((LM_INFO,
                     ACE_TEXT("\n\n==== TEST case 2 : Reliable, Keep All, max samples = 2.\n")
                     ACE_TEXT("Write 1 sample ea. on 3 instances: Should block on third obtain buffer due to max_samples\n")
                     ACE_TEXT("===============================================\n")));

          test->get_default_datawriter_qos(dw_qos);

          dw_qos.history.kind = ::DDS::KEEP_ALL_HISTORY_QOS;
          dw_qos.history.depth = ::DDS::LENGTH_UNLIMITED;

          dw_qos.resource_limits.max_samples = 2;

          Test::SimpleDataWriterImpl* fast_dw = new Test::SimpleDataWriterImpl();
          test->substitute_dw_particpant(fast_dw, tpi);
          WriteDataContainer* test_data_container  = test->get_test_data_container(dw_qos, fast_dw);

          test->log_dw_qos_limits(dw_qos);
          test->log_perceived_qos_limits(test_data_container);
          test->log_send_state_lists("Initial Setup:", test_data_container);

          Test::Simple foo1;
          foo1.key  = 1;
          foo1.count = 1;

          ACE_Message_Block* mb1 = test->dds_marshal(fast_dw, foo1, OpenDDS::DCPS::KEY_ONLY_MARSHALING);

          ::DDS::InstanceHandle_t handle1 = DDS::HANDLE_NIL;

          DDS::ReturnCode_t retval = test_data_container->register_instance(handle1, mb1);
          test->log_send_state_lists("After register instance", test_data_container);

          if (retval != DDS::RETCODE_OK) {
            ACE_ERROR((LM_ERROR, "register instance failed\n"));
          }

          DDS::ReturnCode_t ret;

          //obtain the WriteDataContainer's lock as would be normal
          //when obtained by the datawriter during a write before accessing
          //the WriteDataContainer
          ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
                            guard,
                            test->lock_wdc(test_data_container),
                            ::DDS::RETCODE_ERROR);

          DataSampleElement* element_1 = 0;
          ret = test_data_container->obtain_buffer(element_1, handle1);

          test->log_send_state_lists("After obtain buffer", test_data_container);

          if (ret != DDS::RETCODE_OK) {
            ACE_ERROR((LM_ERROR, "obtain buffer failed for element 1\n"));
          }

          element_1->set_sample(mb1);

          if (ret != DDS::RETCODE_OK) {
            ACE_ERROR((LM_ERROR, "failed to write element 1\n"));
            return ret;
          }

          ret = test_data_container->enqueue(element_1, handle1);
          test->log_send_state_lists("After enqueue", test_data_container);

          SendStateDataSampleList temp;
          test_data_container->get_unsent_data(temp);
          test->log_send_state_lists("After get_unsent_data", test_data_container);

          if (ret != DDS::RETCODE_OK) {
            ACE_ERROR((LM_ERROR, "failed to enqueue element 1\n"));
          }
          Test::Simple foo2;
          foo2.key  = 2;
          foo2.count = 1;

          ACE_Message_Block* mb2 = test->dds_marshal(fast_dw, foo2, OpenDDS::DCPS::KEY_ONLY_MARSHALING);

          ::DDS::InstanceHandle_t handle2 = DDS::HANDLE_NIL;

          retval = test_data_container->register_instance(handle2, mb2);
          test->log_send_state_lists("After register instance 2", test_data_container);


          DataSampleElement* element_2 = 0;
          ret = test_data_container->obtain_buffer(element_2, handle2);
          test->log_send_state_lists("After obtain buffer", test_data_container);


          if (ret != DDS::RETCODE_OK) {
            ACE_ERROR((LM_ERROR, "obtain buffer failed for element 2\n"));
          }

          element_2->set_sample(mb2);

          if (ret != DDS::RETCODE_OK) {
            return ret;
          }

          ret = test_data_container->enqueue(element_2, handle2);
          test->log_send_state_lists("After enqueue", test_data_container);

          temp.reset();
          test_data_container->get_unsent_data(temp);
          test->log_send_state_lists("After get_unsent_data", test_data_container);

          if (ret != DDS::RETCODE_OK) {
            ACE_ERROR((LM_ERROR, "failed to enqueue element 2\n"));
          }
          Test::Simple foo3;
          foo3.key  = 3;
          foo3.count = 1;

          ACE_Message_Block* mb3 = test->dds_marshal(fast_dw, foo3, OpenDDS::DCPS::KEY_ONLY_MARSHALING);

          ::DDS::InstanceHandle_t handle3 = DDS::HANDLE_NIL;

          retval = test_data_container->register_instance(handle3, mb3);
          test->log_send_state_lists("After register instance 3", test_data_container);

          DataSampleElement* element_3 = 0;
          ret = test_data_container->obtain_buffer(element_3, handle3);
          test->log_send_state_lists("After obtain buffer which should block", test_data_container);

          TEST_ASSERT(errno == ETIME);

          test->log_send_state_lists("After TEST_ASSERT timeout", test_data_container);

          test_data_container->unregister_all();
          guard.release();
          delete test_data_container;
          delete fast_dw;
        } //End Test Case 2 scope

        { //Test Case 3 scope
          //=====================================================
          ACE_DEBUG((LM_INFO,
                     ACE_TEXT("\n\n==== TEST case 3 : Reliable, max samples per instance = 1, max samples = 2.\n")
                     ACE_TEXT("Write 1 sample ea. on 3 instances: Should block on third obtain buffer due to max_samples\n")
                     ACE_TEXT("===============================================\n")));

          test->get_default_datawriter_qos(dw_qos);

          dw_qos.resource_limits.max_samples = 2;
          dw_qos.resource_limits.max_samples_per_instance = 1;

          dw_qos.reliability.max_blocking_time.sec = 2;
          dw_qos.reliability.max_blocking_time.nanosec = MAX_BLOCKING_TIME_NANO;

          Test::SimpleDataWriterImpl* fast_dw = new Test::SimpleDataWriterImpl();
          test->substitute_dw_particpant(fast_dw, tpi);
          WriteDataContainer* test_data_container  = test->get_test_data_container(dw_qos, fast_dw);

          test->log_dw_qos_limits(dw_qos);
          test->log_perceived_qos_limits(test_data_container);
          test->log_send_state_lists("Initial Setup:", test_data_container);

          Test::Simple foo1;
          foo1.key  = 1;
          foo1.count = 1;

          ACE_Message_Block* mb1 = test->dds_marshal(fast_dw, foo1, OpenDDS::DCPS::KEY_ONLY_MARSHALING);

          ::DDS::InstanceHandle_t handle1 = DDS::HANDLE_NIL;

          DDS::ReturnCode_t retval = test_data_container->register_instance(handle1, mb1);
          test->log_send_state_lists("After register instance", test_data_container);

          if (retval != DDS::RETCODE_OK) {
            ACE_ERROR((LM_ERROR, "register instance failed\n"));
          }

          DDS::ReturnCode_t ret;

          //obtain the WriteDataContainer's lock as would be normal
          //when obtained by the datawriter during a write before accessing
          //the WriteDataContainer
          ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
                            guard,
                            test->lock_wdc(test_data_container),
                            ::DDS::RETCODE_ERROR);

          DataSampleElement* element_1 = 0;
          ret = test_data_container->obtain_buffer(element_1, handle1);

          test->log_send_state_lists("After obtain buffer", test_data_container);

          if (ret != DDS::RETCODE_OK) {
            ACE_ERROR((LM_ERROR, "obtain buffer failed for element 1\n"));
          }

          element_1->set_sample(mb1);

          if (ret != DDS::RETCODE_OK) {
            ACE_ERROR((LM_ERROR, "failed to write element 1\n"));
            return ret;
          }

          ret = test_data_container->enqueue(element_1, handle1);
          test->log_send_state_lists("After enqueue", test_data_container);

          SendStateDataSampleList temp;
          test_data_container->get_unsent_data(temp);
          test->log_send_state_lists("After get_unsent_data", test_data_container);

          if (ret != DDS::RETCODE_OK) {
            ACE_ERROR((LM_ERROR, "failed to enqueue element 1\n"));
          }
          Test::Simple foo2;
          foo2.key  = 2;
          foo2.count = 1;

          ACE_Message_Block* mb2 = test->dds_marshal(fast_dw, foo2, OpenDDS::DCPS::KEY_ONLY_MARSHALING);

          ::DDS::InstanceHandle_t handle2 = DDS::HANDLE_NIL;

          retval = test_data_container->register_instance(handle2, mb2);
          test->log_send_state_lists("After register instance 2", test_data_container);


          DataSampleElement* element_2 = 0;
          ret = test_data_container->obtain_buffer(element_2, handle2);
          test->log_send_state_lists("After obtain buffer", test_data_container);


          if (ret != DDS::RETCODE_OK) {
            ACE_ERROR((LM_ERROR, "obtain buffer failed for element 2\n"));
          }

          element_2->set_sample(mb2);

          if (ret != DDS::RETCODE_OK) {
            return ret;
          }

          ret = test_data_container->enqueue(element_2, handle2);
          test->log_send_state_lists("After enqueue", test_data_container);

          temp.reset();
          test_data_container->get_unsent_data(temp);
          test->log_send_state_lists("After get_unsent_data", test_data_container);

          if (ret != DDS::RETCODE_OK) {
            ACE_ERROR((LM_ERROR, "failed to enqueue element 2\n"));
          }
          Test::Simple foo3;
          foo3.key  = 3;
          foo3.count = 1;

          ACE_Message_Block* mb3 = test->dds_marshal(fast_dw, foo3, OpenDDS::DCPS::KEY_ONLY_MARSHALING);

          ::DDS::InstanceHandle_t handle3 = DDS::HANDLE_NIL;

          retval = test_data_container->register_instance(handle3, mb3);
          test->log_send_state_lists("After register instance 3", test_data_container);

          DataSampleElement* element_3_first_time_blocks = 0;

          ret = test_data_container->obtain_buffer(element_3_first_time_blocks, handle3);
          test->log_send_state_lists("After 3rd obtain buffer which should block", test_data_container);

          TEST_ASSERT(errno == ETIME);

          test->log_send_state_lists("After TEST_ASSERT 3rd obtain_buffer TIMED OUT", test_data_container);

          test_data_container->unregister_all();

          guard.release();
          delete test_data_container;
          delete fast_dw;
        } //End Test Case 3 scope

        { //Test Case 4 scope
          //=====================================================
          ACE_DEBUG((LM_INFO,
                     ACE_TEXT("\n\n==== TEST case 4 : Reliable, max samples per instance = 2, max samples = 3.\n")
                     ACE_TEXT("Write 3 samples on 1 instance: Should block on third obtain buffer due to max samples per instance\n")
                     ACE_TEXT("===============================================\n")));

          test->get_default_datawriter_qos(dw_qos);

          dw_qos.history.depth = 2;

          dw_qos.resource_limits.max_samples = MAX_SAMPLES;
          dw_qos.resource_limits.max_samples_per_instance = dw_qos.history.depth;

          Test::SimpleDataWriterImpl* fast_dw = new Test::SimpleDataWriterImpl();
          test->substitute_dw_particpant(fast_dw, tpi);
          WriteDataContainer* test_data_container  = test->get_test_data_container(dw_qos, fast_dw);

          test->log_dw_qos_limits(dw_qos);
          test->log_perceived_qos_limits(test_data_container);
          test->log_send_state_lists("Initial Setup:", test_data_container);

          Test::Simple foo1;
          foo1.key  = 1;
          foo1.count = 1;

          ACE_Message_Block* mb = test->dds_marshal(fast_dw, foo1, OpenDDS::DCPS::KEY_ONLY_MARSHALING);

          ::DDS::InstanceHandle_t handle1 = DDS::HANDLE_NIL;//fast_dw->register_instance(foo1);

          DDS::ReturnCode_t retval = test_data_container->register_instance(handle1, mb);
          test->log_send_state_lists("After register instance", test_data_container);

          if (retval != DDS::RETCODE_OK) {
            ACE_ERROR((LM_ERROR, "register instance failed\n"));
          }

          DDS::ReturnCode_t ret;

          //obtain the WriteDataContainer's lock as would be normal
          //when obtained by the datawriter during a write before accessing
          //the WriteDataContainer
          ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
                            guard,
                            test->lock_wdc(test_data_container),
                            ::DDS::RETCODE_ERROR);

          DataSampleElement* element_0 = 0;
          ret = test_data_container->obtain_buffer(element_0, handle1);
          test->log_send_state_lists("After obtain buffer", test_data_container);


          if (ret != DDS::RETCODE_OK) {
            ACE_ERROR((LM_ERROR, "obtain buffer failed for element 0\n"));
          }

          element_0->set_sample(mb);

          if (ret != DDS::RETCODE_OK) {
            ACE_ERROR((LM_ERROR, "failed to write element 0\n"));
            return ret;
          }

          ret = test_data_container->enqueue(element_0, handle1);
          test->log_send_state_lists("After enqueue", test_data_container);

          SendStateDataSampleList temp;
          test_data_container->get_unsent_data(temp);
          test->log_send_state_lists("After get_unsent_data", test_data_container);

          if (ret != DDS::RETCODE_OK) {
            ACE_ERROR((LM_ERROR, "failed to enqueue element 0\n"));
          }
          DataSampleElement* element_1 = 0;
          ret = test_data_container->obtain_buffer(element_1, handle1);
          test->log_send_state_lists("After obtain buffer", test_data_container);

          if (ret != DDS::RETCODE_OK) {
            ACE_ERROR((LM_ERROR, "obtain buffer failed for element 1\n"));
          }
          ACE_Message_Block* mb1 = test->dds_marshal(fast_dw, foo1, OpenDDS::DCPS::KEY_ONLY_MARSHALING);

          element_1->set_sample(mb1);

          if (ret != DDS::RETCODE_OK) {
            return ret;
          }

          ret = test_data_container->enqueue(element_1, handle1);
          test->log_send_state_lists("After enqueue", test_data_container);

          temp.reset();
          test_data_container->get_unsent_data(temp);
          test->log_send_state_lists("After get_unsent_data", test_data_container);

          if (ret != DDS::RETCODE_OK) {
            ACE_ERROR((LM_ERROR, "failed to enqueue element 1\n"));
          }
          DataSampleElement* element_2 = 0;
          ret = test_data_container->obtain_buffer(element_2, handle1);
          test->log_send_state_lists("After obtain buffer which should block", test_data_container);

          TEST_ASSERT(errno == ETIME);

          test->log_send_state_lists("After TEST_ASSERT timeout", test_data_container);

          test_data_container->unregister_all();
          guard.release();
          delete test_data_container;
          delete fast_dw;
        } //End Test Case 4 scope


      }//test scope
      catch (const TestException&)
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT("(%P|%t) TestException caught in main.cpp. ")));
        return 1;
      }
      //======== clean up ============
      delete test;
      delete tpi;
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
  catch (...)
    {
      ACE_ERROR ((LM_ERROR, ACE_TEXT("(%P|%t) unknown exception caught in main.cpp. ")));
      return 1;
    }

  return test_failed;
}

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  int ret = 0;
  try {
    ret = run_test(argc, argv);
  }
  catch (const CORBA::Exception& ex)
    {
      ex._tao_print_exception ("Exception caught in main.cpp:");
      return 1;
    }
  // cleanup
  TheServiceParticipant->shutdown();
  ACE_Thread_Manager::instance()->wait();
  return ret;
}
