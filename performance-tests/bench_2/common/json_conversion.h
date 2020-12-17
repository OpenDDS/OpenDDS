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

template<typename IDL_Type>
bool idl_2_json(const IDL_Type& idl_value, std::ostream& os,
    int max_decimal_places = rapidjson::Writer<rapidjson::OStreamWrapper>::kDefaultMaxDecimalPlaces) {
  rapidjson::Document document;
  document.SetObject();
  OpenDDS::DCPS::copyToRapidJson(idl_value, document, document.GetAllocator());
  rapidjson::OStreamWrapper osw(os);
  rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
  if (max_decimal_places != rapidjson::Writer<rapidjson::OStreamWrapper>::kDefaultMaxDecimalPlaces) {
    writer.SetMaxDecimalPlaces(max_decimal_places);
  }
  document.Accept(writer);
  osw.Flush();
  os << std::endl;
  return true;
}

template<typename IDL_Type>
bool idl_2_pretty_json(const IDL_Type& idl_value, std::ostream& os,
    int max_decimal_places = rapidjson::PrettyWriter<rapidjson::OStreamWrapper>::kDefaultMaxDecimalPlaces) {
  rapidjson::Document document;
  document.SetObject();
  OpenDDS::DCPS::copyToRapidJson(idl_value, document, document.GetAllocator());
  rapidjson::OStreamWrapper osw(os);
  rapidjson::PrettyWriter<rapidjson::OStreamWrapper> pretty_writer(osw);
  if (max_decimal_places != rapidjson::PrettyWriter<rapidjson::OStreamWrapper>::kDefaultMaxDecimalPlaces) {
    pretty_writer.SetMaxDecimalPlaces(max_decimal_places);
  }
  document.Accept(pretty_writer);
  osw.Flush();
  os << std::endl;
  return true;
}

}

#endif
