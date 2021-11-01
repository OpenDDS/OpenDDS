/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/WaitSet.h>
#include <dds/DCPS/Serializer.h>
#include <dds/DCPS/XTypes/TypeObject.h>
#include <dds/DCPS/XTypes/DynamicData.h>
#include "dds/DCPS/StaticIncludes.h"
#if defined ACE_AS_STATIC_LIBS && !defined OPENDDS_SAFETY_PROFILE
#include <dds/DCPS/transport/udp/Udp.h>
#include <dds/DCPS/transport/multicast/Multicast.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#include <dds/DCPS/transport/shmem/Shmem.h>
#endif

#include <dds/DCPS/transport/framework/TransportRegistry.h>
#include <dds/DCPS/transport/framework/TransportConfig.h>
#include <dds/DCPS/transport/framework/TransportInst.h>

#include <dds/DCPS/Recorder.h>

#include <ace/Semaphore.h>
#include <ace/Thread_Semaphore.h>

#include <iostream>
#include <sstream>

int print_dynamic_data(OpenDDS::XTypes::DynamicData dd) {
  ACE_DEBUG((LM_DEBUG, "Type is: \n"));
  OpenDDS::XTypes::TypeKind top_tk = dd.get_type()->get_kind();
  if (top_tk == OpenDDS::XTypes::TK_STRUCTURE) {
    std::cout << "struct " << dd.get_type()->get_name() << " {\n";
    OpenDDS::XTypes::DynamicTypeMembersById dtmbi;
    dd.get_type()->get_all_members(dtmbi);
    for (OpenDDS::XTypes::DynamicTypeMembersById::iterator iter = dtmbi.begin(); iter != dtmbi.end(); ++iter)
    {
      OpenDDS::XTypes::TypeKind member_tk = iter->second->get_descriptor().get_type()->get_descriptor().kind;
      OpenDDS::DCPS::String member_name = iter->second->get_descriptor().name;
      OpenDDS::DCPS::String type_name = iter->second->get_descriptor().get_type()->get_descriptor().name;
      // switch on member kind
      // create type of member
      // get value of member into type
      // print type, name, and value
      if (member_tk == OpenDDS::XTypes::TK_INT32) {
        ACE_CDR::Long my_long;
        if (dd.get_int32_value(my_long, iter->first) != DDS::RETCODE_OK) {
          ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) Error: print_dynamic_data - failed to get_int32_value\n"), -1);
        }
        if (my_long != 5) {
          ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) Error: print_dynamic_data - str: %d does not match expected value: 5\n", my_long), -1);
        }
        std::cout << "  " << type_name << " " << member_name << " = " << std::to_string(my_long) << "\n";
      } else if (member_tk == OpenDDS::XTypes::TK_STRING8) {
        ACE_CDR::Char* my_string;
        if (dd.get_string_value(my_string, iter->first) != DDS::RETCODE_OK) {
          ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) Error: print_dynamic_data - failed to get_string_value\n"), -1);
        }
        if (my_string != std::string("HelloWorld")) {
          ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) Error: print_dynamic_data - str: %C does not match expected value: HelloWorld\n", my_string), -1);
        }
        std::cout << "  " << type_name << " " <<  member_name << " = " << my_string << "\n";
      } else if (member_tk == OpenDDS::XTypes::TK_SEQUENCE) {
        type_name = iter->second->get_descriptor().get_type()->get_descriptor().element_type->get_descriptor().name;
        OpenDDS::XTypes::TypeKind ele_type = iter->second->get_descriptor().get_type()->get_descriptor().element_type->get_kind();
        if (ele_type == OpenDDS::XTypes::TK_ENUM) {
          CORBA::LongSeq my_int_seq;
          if (dd.get_int32_values(my_int_seq, iter->first) != DDS::RETCODE_OK) {
            ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) Error: print_dynamic_data - failed to get_int32_values\n"), -1);
          }
          OpenDDS::XTypes::LBound bound = iter->second->get_descriptor().get_type()->get_descriptor().bound[0];
          OpenDDS::DCPS::String bound_str = bound != 0 ? (", " + std::to_string(bound)) : "";
          if (my_int_seq[0] != 1 || my_int_seq[1] != 0) {
            ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) Error: print_dynamic_data - enum_seq: <%d,%d> does not match expected values: <1,0>\n", my_int_seq[0], my_int_seq[1]), -1);
          }
          std::cout << "  sequence<" << type_name << bound_str << "> " <<  member_name << " = ";
          std::cout << " < " << std::to_string(my_int_seq[0]) << ", ";
          std::cout << std::to_string(my_int_seq[1]) << ">\n";
        }
      } else if (member_tk == OpenDDS::XTypes::TK_ARRAY) {
        type_name = iter->second->get_descriptor().get_type()->get_descriptor().element_type->get_descriptor().name;
        OpenDDS::XTypes::TypeKind ele_type = iter->second->get_descriptor().get_type()->get_descriptor().element_type->get_kind();
        if (ele_type == OpenDDS::XTypes::TK_INT16) {
          OpenDDS::XTypes::DynamicData my_arr_dd;
          if (dd.get_complex_value(my_arr_dd, iter->first) != DDS::RETCODE_OK) {
            ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) Error: print_dynamic_data - failed to get_complex_value\n"), -1);
          }
          OpenDDS::XTypes::LBound bound = iter->second->get_descriptor().get_type()->get_descriptor().bound[0];
          std::cout << "  " << type_name << "[" << bound << "] " <<  member_name << " = ";
          short my_short;
          for (ACE_CDR::ULong i = 0; i < bound; ++i) {
            if (my_arr_dd.get_int16_value(my_short, i) != DDS::RETCODE_OK) {
              ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) Error: print_dynamic_data - failed to get_int16_value\n"), -1);
            }
            if (i == 0 && my_short != 5) {
              ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) Error: print_dynamic_data - short_arr[0]: %d does not match expected value: 5\n", my_short), -1);
            } else if (i == 1 && my_short != 6) {
              ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) Error: print_dynamic_data - short_arr[1]: %d does not match expected value: 6\n", my_short), -1);
            }
            std::cout << " [" << i << "]" << std::to_string(my_short) << ",";
          }
          std::cout << "\n";
        }
      } else if (member_tk == OpenDDS::XTypes::TK_ALIAS) {
        type_name = iter->second->get_descriptor().get_type()->get_descriptor().name;
        OpenDDS::XTypes::TypeKind aliased_type = iter->second->get_descriptor().get_type()->get_descriptor().base_type->get_descriptor().kind;
        OpenDDS::XTypes::TypeKind ele_type = iter->second->get_descriptor().get_type()->get_descriptor().base_type->get_descriptor().element_type->get_kind();
        if (aliased_type == OpenDDS::XTypes::TK_ARRAY) {
          if (ele_type == OpenDDS::XTypes::TK_CHAR8) {
            OpenDDS::XTypes::DynamicData my_arr_dd;
            if (dd.get_complex_value(my_arr_dd, iter->first) != DDS::RETCODE_OK) {
              ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) Error: print_dynamic_data - failed to get_complex_value\n"), -1);
            }
            OpenDDS::XTypes::LBound bound = iter->second->get_descriptor().get_type()->get_descriptor().base_type->get_descriptor().bound[0];
            std::cout << "  " << type_name << " " <<  member_name << " = ";
            ACE_CDR::Char my_char;
            for (ACE_CDR::ULong i = 0; i < bound; ++i) {
              if (my_arr_dd.get_char8_value(my_char, i) != DDS::RETCODE_OK) {
                ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) Error: print_dynamic_data - failed to get_char8_value\n"), -1);
              }
              if (i == 0 && my_char != 'a') {
                ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) Error: print_dynamic_data - char_arr[0]: %c does not match expected value: a\n", my_char), -1);
              } else if (i == 1 && my_char != 'b') {
                ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) Error: print_dynamic_data - char_arr[1]: %c does not match expected value: b\n", my_char), -1);
              }
              std::cout << " [" << i << "]" << my_char << ",";
            }
            std::cout << "\n";
          }
        } else if (aliased_type == OpenDDS::XTypes::TK_SEQUENCE) {
          if (ele_type == OpenDDS::XTypes::TK_BOOLEAN) {
            CORBA::BooleanSeq my_bool_seq;
            if (dd.get_boolean_values(my_bool_seq, iter->first) != DDS::RETCODE_OK) {
              ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) Error: print_dynamic_data - failed to get_boolean_values\n"), -1);
            }
            if (my_bool_seq[0] != true || my_bool_seq[1] != false) {
              ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) Error: print_dynamic_data - bool_seq: <%d,%d> does not match expected values: <1,0>\n",
                                my_bool_seq[0], my_bool_seq[1]), -1);
            }
            std::cout << "  " << type_name << " " <<  member_name << " = ";
            std::cout << " < " << std::to_string(my_bool_seq[0]) << ", ";
            std::cout << std::to_string(my_bool_seq[1]) << ">\n";
          }
        }
      } else if (member_tk == OpenDDS::XTypes::TK_STRUCTURE) {
        OpenDDS::XTypes::DynamicData nested_dd;
        if (dd.get_complex_value(nested_dd, iter->first) != DDS::RETCODE_OK) {
          ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) Error: print_dynamic_data - failed to get_complex_value\n"), -1);
        }
        OpenDDS::XTypes::DynamicTypeMembersById nested_dtmbi;
        nested_dd.get_type()->get_all_members(nested_dtmbi);
        std::cout << "  struct " << nested_dd.get_type()->get_name() << " {\n";
        for (OpenDDS::XTypes::DynamicTypeMembersById::iterator nested_iter = nested_dtmbi.begin(); nested_iter != nested_dtmbi.end(); ++nested_iter)
        {
          OpenDDS::XTypes::TypeKind nested_member_tk = nested_iter->second->get_descriptor().get_type()->get_descriptor().kind;
          OpenDDS::DCPS::String nested_member_name = nested_iter->second->get_descriptor().name;
          OpenDDS::DCPS::String nested_type_name = nested_iter->second->get_descriptor().get_type()->get_descriptor().name;
          if (nested_member_tk == OpenDDS::XTypes::TK_INT32) {
            ACE_CDR::Long my_long;     
            if (nested_dd.get_int32_value(my_long, 0) != DDS::RETCODE_OK) {
              ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) Error: print_dynamic_data - failed to get_int32_value\n"), -1);
            }
            if (my_long != 5) {
              ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) Error: print_dynamic_data - inner_struct long: %d did not equal: 5\n", my_long), -1);
            }
            std::cout << "    " << nested_type_name << " " << nested_member_name << " = ";
            std::cout << "[0]" << std::to_string(my_long) << "\n  }\n";
          }
        }
      }
    }
    std::cout << "}\n";
  }
  return 0;
}


class TestRecorderListener : public OpenDDS::DCPS::RecorderListener
{
public:
  explicit TestRecorderListener()
    : sem_(0),
      ret_val_(0)
  {
  }

  virtual void on_sample_data_received(OpenDDS::DCPS::Recorder* rec,
                                       const OpenDDS::DCPS::RawDataSample& sample)
  {
    using namespace OpenDDS::DCPS;
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("TestRecorderListener::on_sample_data_received\n")));
    OpenDDS::XTypes::DynamicData dd = rec->get_dynamic_data(sample);
    ret_val_ = print_dynamic_data(dd);
  }

  virtual void on_recorder_matched(OpenDDS::DCPS::Recorder*,
                                   const ::DDS::SubscriptionMatchedStatus& status )
  {
    if (status.current_count == 1) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("TestRecorderListener -- a writer connect to recorder\n")));
    }
    else if (status.current_count == 0 && status.total_count > 0) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("TestRecorderListener -- writer disconnect with recorder\n")));
      sem_.release();
    }
  }

  int wait(const ACE_Time_Value & tv) {
    ACE_Time_Value timeout = ACE_OS::gettimeofday() + tv;
    return sem_.acquire(timeout);
  }
  int ret_val_;
private:
  ACE_Thread_Semaphore sem_;
};


int run_test(int argc, ACE_TCHAR *argv[]){
  int ret_val = 0;
  try {
    // OpenDDS::DCPS::String type_name = "stru";
    OpenDDS::DCPS::String type_name = argv[1];
    // Initialize DomainParticipantFactory
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);

    OpenDDS::DCPS::Service_Participant* service = TheServiceParticipant;

    // Create DomainParticipant
    DDS::DomainParticipant_var participant =
      dpf->create_participant(153,
                              PARTICIPANT_QOS_DEFAULT,
                              0,
                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!participant) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("ERROR: %N:%l: main() -")
                        ACE_TEXT(" create_participant failed!\n")),
                       -1);
    }
    using namespace OpenDDS::DCPS;

    {
      DDS::Topic_var topic =
        service->create_typeless_topic(participant,
                                       "recorder_topic",
                                       type_name.c_str(),
                                       true,
                                       TOPIC_QOS_DEFAULT,
                                       0,
                                       OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (!topic) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("ERROR: %N:%l: main() -")
                          ACE_TEXT(" create_topic failed!\n")),
                         -1);
      }

      ACE_Time_Value wait_time(60, 0);

      RcHandle<TestRecorderListener> recorder_listener = make_rch<TestRecorderListener> ();

      DDS::SubscriberQos sub_qos;
      participant->get_default_subscriber_qos(sub_qos);

      DDS::DataReaderQos dr_qos = service->initial_DataReaderQos();
      dr_qos.representation.value.length(1);
      dr_qos.representation.value[0] = DDS::XCDR2_DATA_REPRESENTATION;
      // Create Recorder
      OpenDDS::DCPS::Recorder_var recorder =
        service->create_recorder(participant,
                                 topic.in(),
                                 sub_qos,
                                 dr_qos,
                                 recorder_listener);

      if (!recorder.in()) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("ERROR: %N:%l: main() -")
                          ACE_TEXT(" create_recorder failed!\n")),
                         -1);
      }


      // wait until the writer disconnnects
      if (recorder_listener->wait(wait_time) == -1) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("ERROR: %N:%l: main() -")
                          ACE_TEXT(" recorder timeout!\n")),
                         -1);
      }
      ret_val = recorder_listener->ret_val_;
      service->delete_recorder(recorder);
    }
    participant->delete_contained_entities();
    dpf->delete_participant(participant);
    TheServiceParticipant->shutdown();
  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    return -1;
  }
  if (ret_val == -1) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("ERROR: %N:%l: main() -")
                      ACE_TEXT(" failed to properly analyze sample!\n")),
                     -1);
  }
  return ret_val;
}

int
ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  int ret = run_test(argc, argv);
  ACE_Thread_Manager::instance()->wait();
  return ret;
}
