#include "DataReaderQos_Handler.h"
#include "QOS_DataReader_T.h"
#include <memory>
#include <string>
#include "dds/DCPS/debug.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

bool
DataReaderQos_Handler::get_datareader_qos(DDS::DataReaderQos& dr_qos,
                                          const dds::qosProfile * profile,
                                          std::string_view name)
{
  if (!name.empty())
    {
//       // find the correct DataReader_qos
//       for (dds::qosProfile::DataReader_qos_iterator dw_it = profile->begin_DataReader_qos();
//           dw_it != profile->end_DataReader_qos();
//           ++dw_it)
//         {
//           if (ACE_OS::strcmp(dw_name.c_str(), name) == 0)
//             {
//               return get_datareader_qos(dr_qos, std::addressof(datareader_qos));
//             }
//         }
    }
  else
    {
      if (profile->datareader_qos().empty())
        {
          if (OpenDDS::DCPS::DCPS_debug_level > 7)
            {
              ACE_DEBUG((LM_TRACE,
                ACE_TEXT("DataReaderQos_Handler::get_datareader_qos - ")
                ACE_TEXT("No DataReader QOS available in profile <%C>\n"),
                profile->name().c_str()));
            }
          return true;
        }
      // get the first DataReader in the XML
      const dds::datareaderQos& datareader_qos = profile->datareader_qos().front();
      return DataReaderQos_Handler::get_datareader_qos(dr_qos, std::addressof(datareader_qos));
    }

  const std::string name_string (name);
  ACE_ERROR((LM_ERROR,
    ACE_TEXT("(%P|%t) ERROR: DataReaderQos_Handler::get_datareader_qos - ")
    ACE_TEXT("Unable to find DataReaderQos <%C>\n"),
    name_string.c_str()));
  return false;
}

bool
DataReaderQos_Handler::get_datareader_qos(DDS::DataReaderQos& dr_qos,
                                          const dds::datareaderQos * dr)
{
  typedef QOS_DataReader_T<const dds::datareaderQos*, DDS::DataReaderQos> dr_type;
  dr_type datareader_qos_handler;
  datareader_qos_handler.read_qos(dr_qos, dr);

  return true;
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
