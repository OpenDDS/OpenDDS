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


bool print_dynamic_data(OpenDDS::XTypes::DynamicData dd, OpenDDS::XTypes::DynamicType_rch dt, OpenDDS::DCPS::String& type_string, OpenDDS::DCPS::String indent)
{
  OpenDDS::DCPS::String member_name;
  OpenDDS::DCPS::String type_name;
  OpenDDS::XTypes::DynamicData temp_dd;
  OpenDDS::XTypes::DynamicType_rch temp_dt;
  switch (dt->get_kind()) {
  case OpenDDS::XTypes::TK_INT8: {
    ACE_CDR::Int8 my_int8;
    if (dd.get_int8_value(my_int8, 0) != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) Error: print_dynamic_data - failed to get_int8_value\n"), false);
    }
    type_string += " = " + std::to_string(my_int8) + "\n";
    break;
  }
  case OpenDDS::XTypes::TK_UINT8: {
    ACE_CDR::UInt8 my_uint8;
    if (dd.get_uint8_value(my_uint8, 0) != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) Error: print_dynamic_data - failed to get_uint8_value\n"), false);
    }
    type_string += " = " + std::to_string(my_uint8) + "\n";
    break;
  }
  case OpenDDS::XTypes::TK_INT16: {
    ACE_CDR::Short my_short;
    if (dd.get_int16_value(my_short, 0) != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) Error: print_dynamic_data - failed to get_int16_value\n"), false);
    }
    type_string += " = " + std::to_string(my_short) + "\n";
    break;
  }
  case OpenDDS::XTypes::TK_UINT16: {
    ACE_CDR::UShort my_ushort;
    if (dd.get_uint16_value(my_ushort, 0) != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) Error: print_dynamic_data - failed to get_uint16_value\n"), false);
    }
    type_string += " = " + std::to_string(my_ushort) + "\n";
    break;
  }
  case OpenDDS::XTypes::TK_ENUM:
  case OpenDDS::XTypes::TK_INT32: {
    ACE_CDR::Long my_long;
    if (dd.get_int32_value(my_long, 0) != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) Error: print_dynamic_data - failed to get_int32_value\n"), false);
    }
    type_string += " = " + std::to_string(my_long) + "\n";
    break;
  }
  case OpenDDS::XTypes::TK_UINT32: {
    ACE_CDR::ULong my_ulong;
    if (dd.get_uint32_value(my_ulong, 0) != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) Error: print_dynamic_data - failed to get_uint32_value\n"), false);
    }
    type_string += " = " + std::to_string(my_ulong) + "\n";
    break;
  }
  case OpenDDS::XTypes::TK_INT64: {
    ACE_CDR::LongLong my_longlong;
    if (dd.get_int64_value(my_longlong, 0) != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) Error: print_dynamic_data - failed to get_int64_value\n"), false);
    }
    type_string += " = " + std::to_string(my_longlong) + "\n";
    break;
  }
  case OpenDDS::XTypes::TK_UINT64: {
    ACE_CDR::ULongLong my_ulonglong;
    if (dd.get_uint64_value(my_ulonglong, 0) != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) Error: print_dynamic_data - failed to get_uint64_value\n"), false);
    }
    type_string += " = " + std::to_string(my_ulonglong) + "\n";
    break;
  }
  case OpenDDS::XTypes::TK_FLOAT32: {
    ACE_CDR::Float my_float;
    if (dd.get_float32_value(my_float, 0) != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) Error: print_dynamic_data - failed to get_float32_value\n"), false);
    }
    type_string += " = " + std::to_string(my_float) + "\n";
    break;
  }
  case OpenDDS::XTypes::TK_FLOAT64: {
    ACE_CDR::Double my_double;
    if (dd.get_float64_value(my_double, 0) != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) Error: print_dynamic_data - failed to get_float64_value\n"), false);
    }
    type_string += " = " + std::to_string(my_double) + "\n";
    break;
  }
  case OpenDDS::XTypes::TK_FLOAT128: {
    ACE_CDR::LongDouble my_longdouble;
    if (dd.get_float128_value(my_longdouble, 0) != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) Error: print_dynamic_data - failed to get_float128_value\n"), false);
    }
    type_string += " = " + std::to_string(my_longdouble) + "\n";
    break;
  }
  case OpenDDS::XTypes::TK_BOOLEAN: {
    ACE_CDR::Boolean my_bool;
    if (dd.get_boolean_value(my_bool, 0) != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) Error: print_dynamic_data - failed to get_boolean_value\n"), false);
    }
    type_string += " = " + std::to_string(my_bool) + "\n";
    break;
  }
  case OpenDDS::XTypes::TK_BYTE: {
    ACE_CDR::Octet my_byte;
    if (dd.get_byte_value(my_byte, 0) != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) Error: print_dynamic_data - failed to get_byte_value\n"), false);
    }
    type_string += " = " + std::to_string(my_byte) + "\n";
    break;
  }
  case OpenDDS::XTypes::TK_CHAR8: {
    ACE_CDR::Char my_char;
    if (dd.get_char8_value(my_char, 0) != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) Error: print_dynamic_data - failed to get_char8_value\n"), false);
    }
    type_string += OpenDDS::DCPS::String(" = \'") + my_char + "\'\n";
    break;
  }
  case OpenDDS::XTypes::TK_CHAR16: {
    ACE_CDR::WChar my_wchar;
    if (dd.get_char16_value(my_wchar, 0) != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) Error: print_dynamic_data - failed to get_char16_value\n"), false);
    }
    type_string += " = \'" + std::to_string(my_wchar) + "\'\n";
    break;
  }
  case OpenDDS::XTypes::TK_STRING8: {
    ACE_CDR::Char* my_string = 0;
    if (dd.get_string_value(my_string, 0) != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) Error: print_dynamic_data - failed to get_string_value\n"), false);
    }
    type_string += OpenDDS::DCPS::String(" = \"") + my_string + "\"\n";
    break;
  }
  case OpenDDS::XTypes::TK_STRING16: {
    ACE_CDR::WChar* my_wstring = 0;
    if (dd.get_wstring_value(my_wstring, 0) != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) Error: print_dynamic_data - failed to get_wstring_value\n"), false);
    }
    //type_string += " = \"" + my_wstring + "\"\n"; TODO CLAYTON: Find how to cast WChar* to a string
    break;
  }
  // case OpenDDS::XTypes::TK_BITMASK:
  // case OpenDDS::XTypes::TK_BITSET:
  case OpenDDS::XTypes::TK_ALIAS: {
    if (!print_dynamic_data(dd, dt->get_descriptor().base_type, type_string, indent)) {
      ACE_ERROR((LM_ERROR, "(%P|%t) print_dynamic_data: failed to read alias\n"));
    }
    break;
  }
  case OpenDDS::XTypes::TK_SEQUENCE: {
    OpenDDS::DCPS::String temp_indent = indent;
    indent += "  ";
    type_name = dt->get_descriptor().element_type->get_descriptor().name;
    member_name = dt->get_descriptor().name;
    ACE_CDR::ULong seq_length = dd.get_item_count();
    type_string += "  " + type_name + "[" + std::to_string(seq_length) + "] " +  member_name + " =\n";
    for (ACE_CDR::ULong i = 0; i < seq_length; ++i) {
      type_string += indent + "[" + std::to_string(i) + "]";
      if (dd.get_complex_value(temp_dd, i) != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) Error: print_dynamic_data - failed to get_complex_value\n"), -1);
      }
      if (!print_dynamic_data(temp_dd, dt->get_descriptor().element_type, type_string, indent)) {
        ACE_ERROR((LM_ERROR, "(%P|%t) print_dynamic_data: failed to read struct member\n"));
      }
    }
    indent = temp_indent;
    break;
  }
  case OpenDDS::XTypes::TK_ARRAY: {
    OpenDDS::DCPS::String temp_indent = indent;
    indent += "  ";
    type_name = dt->get_descriptor().element_type->get_descriptor().name;
    member_name = dt->get_descriptor().name;
    OpenDDS::XTypes::LBound bound = dt->get_descriptor().bound[0];
    type_string += "  " + type_name + "[" + std::to_string(bound) + "] " +  member_name + " =\n";
    for (ACE_CDR::ULong i = 0; i < bound; ++i) {
      type_string += indent + "[" + std::to_string(i) + "]";
      if (dd.get_complex_value(temp_dd, i) != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) Error: print_dynamic_data - failed to get_complex_value\n"), -1);
      }
      if (!print_dynamic_data(temp_dd, dt->get_descriptor().element_type, type_string, indent)) {
        ACE_ERROR((LM_ERROR, "(%P|%t) print_dynamic_data: failed to read struct member\n"));
      }
    }
    indent = temp_indent;
    break;
  }
  case OpenDDS::XTypes::TK_STRUCTURE: {
    OpenDDS::DCPS::String temp_indent = indent;
    indent += "  ";
    type_string += "struct " + dd.get_type()->get_name() + " {\n";
    OpenDDS::XTypes::DynamicTypeMembersById dtmbi;
    dd.get_type()->get_all_members(dtmbi);
    for (OpenDDS::XTypes::DynamicTypeMembersById::iterator iter = dtmbi.begin(); iter != dtmbi.end(); ++iter) {
      dd.get_complex_value(temp_dd, iter->first);
      member_name = iter->second->get_descriptor().name;
      type_name = iter->second->get_descriptor().get_type()->get_descriptor().name;
      type_string += indent + type_name + " " + member_name;
      temp_dt = iter->second->get_descriptor().get_type();
      if (!print_dynamic_data(temp_dd, temp_dt, type_string, indent)) {
        ACE_ERROR((LM_ERROR, "(%P|%t) print_dynamic_data: failed to read struct member\n"));
      }
    }
    indent = temp_indent;
    type_string += indent + "};\n";
    break;
  }
  case OpenDDS::XTypes::TK_UNION: {
    OpenDDS::DCPS::String temp_indent = indent;
    indent += "  ";
    type_string += "union " + dd.get_type()->get_name() + " {\n";
    ACE_CDR::ULong item_count = dd.get_item_count();
    member_name = dt->get_descriptor().discriminator_type->get_descriptor().name;
    type_string += indent + member_name + " DISCRIMINATOR";
    if (dd.get_complex_value(temp_dd, OpenDDS::XTypes::MEMBER_ID_INVALID) != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) Error: print_dynamic_data - failed to get_complex_value\n"), -1);
    }
    if (!print_dynamic_data(temp_dd, dt->get_descriptor().discriminator_type, type_string, indent)) {
      ACE_ERROR((LM_ERROR, "(%P|%t) print_dynamic_data: failed to read struct member\n"));
    }
    if (dd.get_complex_value(temp_dd, 0) != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) Error: print_dynamic_data - failed to get_complex_value\n"), -1);
    }
    if (item_count == 2) {
      //TODO CLAYTON: This is currently hard coded, discuss soltuions with team
      OpenDDS::XTypes::DynamicTypeMember_rch temp_dtm;
      dt->get_member(temp_dtm, 0);
      member_name = temp_dtm->get_descriptor().name;
      type_name = temp_dtm->get_descriptor().get_type()->get_descriptor().name;
      type_string += indent + type_name + " " + member_name;
      if (!print_dynamic_data(temp_dd, temp_dtm->get_descriptor().get_type(), type_string, indent)) {
        ACE_ERROR((LM_ERROR, "(%P|%t) print_dynamic_data: failed to read struct member\n"));
      }
    }
    indent = temp_indent;
    type_string += indent + "};\n";
  }

  }
  return true;
}

int print_dynamic_data(OpenDDS::XTypes::DynamicData dd) {
  ACE_DEBUG((LM_DEBUG, "Type is:\n"));
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
        ACE_CDR::Char* my_string = 0;
        if (dd.get_string_value(my_string, iter->first) != DDS::RETCODE_OK) {
          ACE_ERROR_RETURN((LM_ERROR, "(%P|%t) Error: print_dynamic_data - failed to get_string_value\n"), -1);
        }
        if (my_string != OpenDDS::DCPS::String("HelloWorld")) {
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
    //ret_val_ = print_dynamic_data(dd);
    OpenDDS::DCPS::String my_type = "";
    if (!print_dynamic_data(dd, dd.get_type(), my_type, "")){
      ACE_ERROR((LM_ERROR, " FAILED TEST\n"));
    }
    std::cout << my_type;
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
