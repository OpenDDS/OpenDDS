#include "json_2_builder.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#include "../idl/StoolTypeSupportImpl.h"

#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#pragma GCC diagnostic pop

bool json_2_builder(std::istream& is, Stool::WorkerConfig& config) {
  rapidjson::Document document;
  rapidjson::IStreamWrapper isw(is);
  document.ParseStream(isw);
  if (!document.IsObject())
  {
    std::cerr << "Expected configuration file to contain JSON document object" << std::endl;
    return false;
  }

  OpenDDS::DCPS::copyFromRapidJson(document, config);

  return true;
}

