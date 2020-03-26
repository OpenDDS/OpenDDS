#ifndef COMMON_JSON_CONVERSION_HEADER
#define COMMON_JSON_CONVERSION_HEADER

#include <iostream>

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#endif
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/ostreamwrapper.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

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
bool idl_2_json(const IDL_Type& idl_value, std::ostream& os, bool pretty = false) {
  rapidjson::Document document;
  document.SetObject();
  OpenDDS::DCPS::copyToRapidJson(idl_value, document, document.GetAllocator());
  rapidjson::OStreamWrapper osw(os);
  rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
  rapidjson::PrettyWriter<rapidjson::OStreamWrapper> pretty_writer(osw);
  if (pretty) { // Tried this with a ternary but that doesn't work right.
    document.Accept(pretty_writer);
  } else {
    document.Accept(writer);
  }
  osw.Flush();
  os << std::endl;
  return true;
}

}

#endif
