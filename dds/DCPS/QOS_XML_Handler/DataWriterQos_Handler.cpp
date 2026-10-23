#include "DataWriterQos_Handler.h"
#include "QOS_DataWriter_T.h"
#include <memory>
#include <string>
#include "dds/DCPS/debug.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

bool
DataWriterQos_Handler::get_datawriter_qos(DDS::DataWriterQos& dw_qos,
                                          const dds::qosProfile * profile,
                                          std::string_view name)
{
  if (!name.empty())
    {
//       // find the correct datawriter_qos
//       for (const dds::datawriterQos& datawriter_qos = profile->datawriter_qos().front();
//           dw_it != profile->end_datawriter_qos();
//           ++dw_it)
//         {
//           if (ACE_OS::strcmp(dw_name.c_str(), name) == 0)
//             {
//               return get_datawriter_qos(dw_qos, *&datawriter_qos);
//             }
//         }
    }
  else
    {
      if (profile->datawriter_qos().empty())
        {
          if (OpenDDS::DCPS::DCPS_debug_level > 7)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("DataWriterQos_Handler::get_datawriter_qos - ")
                ACE_TEXT("No DataWriter QOS available in profile <%s>\n"),
                profile->name().c_str()));
            }
          return true;
        }
      // get the first datawriter in the XML
      const dds::datawriterQos& datawriter_qos = profile->datawriter_qos().front();
      return DataWriterQos_Handler::get_datawriter_qos(dw_qos, std::addressof(datawriter_qos));
    }

  const std::string name_string (name);
  ACE_ERROR((LM_ERROR,
    ACE_TEXT("(%P|%t) ERROR: DataWriterQos_Handler::get_datawriter_qos - ")
    ACE_TEXT("Unable to find DataWriterQos <%C>\n"),
    name_string.c_str()));
  return false;
}

bool
DataWriterQos_Handler::get_datawriter_qos(DDS::DataWriterQos& dw_qos,
                                          const dds::datawriterQos * dw)
{
  typedef QOS_DataWriter_T<const dds::datawriterQos*, DDS::DataWriterQos> dw_type;
  dw_type datawriter_qos_handler;
  datawriter_qos_handler.read_qos(dw_qos, dw);

  return true;
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
