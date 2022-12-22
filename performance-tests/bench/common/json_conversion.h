#ifndef COMMON_JSON_CONVERSION_HEADER
#define COMMON_JSON_CONVERSION_HEADER

#include <iostream>

#include <dds/DCPS/JsonValueReader.h>
#include <dds/DCPS/JsonValueWriter.h>

namespace Bench {

template<typename IDL_Type>
bool json_2_idl(std::istream& is, IDL_Type& idl_value) {
  rapidjson::IStreamWrapper isw(is);
  return OpenDDS::DCPS::from_json(idl_value, isw);
}

template<typename IDL_Type, typename Writer_Type = rapidjson::Writer<rapidjson::OStreamWrapper> >
bool idl_2_json(const IDL_Type& idl_value, std::ostream& os,
    int max_decimal_places = Writer_Type::kDefaultMaxDecimalPlaces) {
  rapidjson::OStreamWrapper osw(os);
  Writer_Type writer(osw);
  OpenDDS::DCPS::JsonValueWriter<Writer_Type> jvw(writer);
  if (max_decimal_places != Writer_Type::kDefaultMaxDecimalPlaces) {
    writer.SetMaxDecimalPlaces(max_decimal_places);
  }
  vwrite(jvw, idl_value);
  osw.Flush();
  os << std::endl;
  return true;
}

}

#endif
