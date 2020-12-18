#ifndef COMMON_JSON_CONVERSION_HEADER
#define COMMON_JSON_CONVERSION_HEADER

#include <iostream>

#include <dds/DCPS/RapidJsonWrapper.h>

namespace Bench {

template<typename IDL_Type>
bool json_2_idl(std::istream& is, IDL_Type& idl_value) {
  rapidjson::Document document;
  rapidjson::IStreamWrapper isw(is);
  document.ParseStream(isw);
  if (!document.IsObject()) {
    std::cerr << "Input isn't valid JSON" << std::endl;
    return false;
  }

  OpenDDS::DCPS::copyFromRapidJson(document, idl_value);

  return true;
}

template<typename IDL_Type, typename Writer_Type = rapidjson::Writer<rapidjson::OStreamWrapper> >
bool idl_2_json(const IDL_Type& idl_value, std::ostream& os,
    int max_decimal_places = Writer_Type::kDefaultMaxDecimalPlaces) {
  rapidjson::Document document;
  document.SetObject();
  OpenDDS::DCPS::copyToRapidJson(idl_value, document, document.GetAllocator());
  rapidjson::OStreamWrapper osw(os);
  Writer_Type writer(osw);
  if (max_decimal_places != Writer_Type::kDefaultMaxDecimalPlaces) {
    writer.SetMaxDecimalPlaces(max_decimal_places);
  }
  document.Accept(writer);
  osw.Flush();
  os << std::endl;
  return true;
}

}

#endif
