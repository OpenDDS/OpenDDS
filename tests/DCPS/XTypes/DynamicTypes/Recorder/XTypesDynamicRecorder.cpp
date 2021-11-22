/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <dds/DCPS/GuardCondition.h>
#include <dds/DCPS/WaitSet.h>
#include <dds/DCPS/Recorder.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/XTypes/DynamicData.h>
#if defined ACE_AS_STATIC_LIBS && !defined OPENDDS_SAFETY_PROFILE
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include <ace/Semaphore.h>
#include <ace/Thread_Semaphore.h>

#include <iostream>
#include <sstream>

using namespace OpenDDS::DCPS;
bool sample_read = false;
class TestRecorderListener : public RecorderListener
{
public:
  explicit TestRecorderListener(DDS::GuardCondition_var gc)
    : ret_val_(0),
      sem_(0),
      gc_(gc)
  {
  }

  virtual void on_sample_data_received(Recorder* rec,
                                       const RawDataSample& sample)
  {
    OpenDDS::XTypes::DynamicData dd = rec->get_dynamic_data(sample);
    String my_type = "";
    String indent = "";
    if (!OpenDDS::XTypes::print_dynamic_data(dd, my_type, indent)) {
      if (log_level >= LogLevel::Error) {
        ACE_ERROR((LM_ERROR, "(%P|%t) Error: TestRecorderListener::on_sample_data_received:"
          " Failed to read DynamicData\n"));
      }
    }
    String struct_string =
      "struct ::Dynamic::my_struct {\n"
      "  Int8 my_int8 = 1\n"
      "  UInt8 my_uint8 = 2\n"
      "  Int16 my_short = 3\n"
      "  UInt16 my_ushort = 4\n"
      "  Int32 my_long = 5\n"
      "  UInt32 my_ulong = 6\n"
      "  Int64 my_longlong = 7\n"
      "  UInt64 my_ulonglong = 8\n"
      "  Float32 my_float = 9.25\n"
      "  Float64 my_double = 10.5\n"
      "  Float128 my_longdouble = 11.075\n"
      "  Boolean my_boolean = true\n"
      "  Byte my_byte = 0x0c\n"
      "  Char8 my_char = 'd'\n"
      "  Char16 my_wchar = L'e'\n"
      "  String8 my_string = \"Hello\"\n"
      "  WString16 my_wstring = L\"World\"\n"
      "  Sequence my_alias_seq Boolean[2] =\n"
      "    [0] = true\n"
      "    [1] = false\n"
      "  Array my_alias_array Char8[2] =\n"
      "    [0] = 'a'\n"
      "    [1] = 'b'\n"
      "  Sequence my_anon_seq ::Dynamic::EnumType[2] =\n"
      "    [0] = V2\n"
      "    [1] = V1\n"
      "  Array my_anon_arr Int16[2] =\n"
      "    [0] = 5\n"
      "    [1] = 6\n"
      "};\n";
    String nested_struct_string =
      "struct ::Dynamic::outer_struct {\n"
      "  struct ::Dynamic::inner_struct {\n"
      "    union ::Dynamic::inner_union {\n"
      "      Int32 discriminator = 2\n"
      "      ::Dynamic::bool_seq my_alias_seq Boolean[2] =\n"
      "        [0] = false\n"
      "        [1] = true\n"
      "    };\n"
      "  };\n"
      "};\n";
    String default_union_string =
      "union ::Dynamic::inner_union {\n"
      "  Int32 discriminator = -2147483647\n"
      "  Boolean b = true\n"
      "};\n";
    String nested_union_string =
      "union ::Dynamic::outer_union {\n"
      "  ::Dynamic::EnumType discriminator = V1\n"
      "  struct ::Dynamic::inner_struct {\n"
      "    union ::Dynamic::inner_union {\n"
      "      Int32 discriminator = 1\n"
      "      Int32 l = 5\n"
      "    };\n"
      "  };\n"
      "};\n";

    if (my_type != struct_string &&
        my_type != nested_struct_string &&
        my_type != default_union_string &&
        my_type != nested_union_string &&
        log_level >= LogLevel::Error) {
       ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: TestRecorderListener::on_sample_data_received:"
         " Type did not match\n"));
    }
    std::cout << my_type << "\n";
    gc_->set_trigger_value(true);
  }

  virtual void on_recorder_matched(Recorder*,
                                   const ::DDS::SubscriptionMatchedStatus& status )
  {
    if (status.current_count == 1 && DCPS_debug_level >= 4) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) TestRecorderListener::on_recorder_matched:"
          " a writer connected to recorder\n")));
    } else if (status.current_count == 0 && status.total_count > 0) {
      if (DCPS_debug_level >= 4) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) TestRecorderListener::on_recorder_matched:"
          " a writer disconnected with recorder\n")));
      }
      sem_.release();
    }
  }

  int wait(const ACE_Time_Value & tv)
  {
    ACE_Time_Value timeout = ACE_OS::gettimeofday() + tv;
    return sem_.acquire(timeout);
  }
  int ret_val_;
private:
  ACE_Thread_Semaphore sem_;
  DDS::GuardCondition_var gc_;
};


int run_test(int argc, ACE_TCHAR* argv[])
{
  int ret_val = 0;
  try {
    const ACE_TCHAR* type_name = argv[1];
    // Initialize DomainParticipantFactory
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);

    Service_Participant* service = TheServiceParticipant;

    // Create DomainParticipant
    DDS::DomainParticipant_var participant =
      dpf->create_participant(153,
                              PARTICIPANT_QOS_DEFAULT,
                              0,
                              DEFAULT_STATUS_MASK);

    if (!participant) {
      if (log_level >= LogLevel::Error) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: main(): create_participant failed!\n"));
      }
      return 1;
    }
    DDS::GuardCondition_var gc = new DDS::GuardCondition;
    DDS::WaitSet_var ws = new DDS::WaitSet;
    DDS::ReturnCode_t ret = ws->attach_condition(gc);
    {
      DDS::Topic_var topic =
        service->create_typeless_topic(participant,
                                       "recorder_topic",
                                       ACE_TEXT_ALWAYS_CHAR(type_name),
                                       true,
                                       TOPIC_QOS_DEFAULT,
                                       0,
                                       DEFAULT_STATUS_MASK);

      if (!topic) {
        if (log_level >= LogLevel::Error) {
          ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: main(): create_topic failed!\n"));
        }
        return 1;
      }

      ACE_Time_Value wait_time(60, 0);

      RcHandle<TestRecorderListener> recorder_listener = make_rch<TestRecorderListener> (gc);

      DDS::SubscriberQos sub_qos;
      participant->get_default_subscriber_qos(sub_qos);

      DDS::DataReaderQos dr_qos = service->initial_DataReaderQos();
      dr_qos.representation.value.length(1);
      dr_qos.representation.value[0] = DDS::XCDR2_DATA_REPRESENTATION;
      dr_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
      // Create Recorder
      Recorder_var recorder =
        service->create_recorder(participant,
                                 topic.in(),
                                 sub_qos,
                                 dr_qos,
                                 recorder_listener);

      if (!recorder.in()) {
        if (log_level >= LogLevel::Error) {
          ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: main(): create_recorder failed!\n"));
        }
        return 1;
      }
      DDS::Duration_t timeout = { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC };
      DDS::ConditionSeq conditions;
      ret = ws->wait(conditions, timeout);

      ret_val = recorder_listener->ret_val_;
      if (ret != DDS::RETCODE_OK) {
        if (log_level >= LogLevel::Error) {
          ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: main(): wait failed!\n"));
        }
        return 1;
      }

      service->delete_recorder(recorder);
    }
    participant->delete_contained_entities();
    dpf->delete_participant(participant);
    TheServiceParticipant->shutdown();
  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    return 1;
  }
  if (ret_val == 1 && log_level >= LogLevel::Error) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: main(): failed to properly analyze sample!\n"));
  }
  return ret_val;
}

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  int ret = run_test(argc, argv);
  ACE_Thread_Manager::instance()->wait();
  return ret;
}
