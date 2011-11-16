/*
 * $Id$
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef PARAMETER_LIST_CONVERTER_H
#define PARAMETER_LIST_CONVERTER_H

#include "dds/DCPS/RTPS/rtps_export.h"
#include "dds/DCPS/RTPS/RtpsMessageTypesC.h"

namespace OpenDDS {
namespace RTPS {
  // Converter to and from parameter lists
  class OpenDDS_Rtps_Export ParameterListConverter {
  public:
    int to_param_list(const SPDPdiscoveredParticipantData& participant_data,
                      ParameterList& param_list) const;
    int to_param_list(const DiscoveredWriterData& writer_data,
                      ParameterList& param_list) const;
    int from_param_list(const ParameterList& param_list,
                        SPDPdiscoveredParticipantData& participant_data) const;
  private:
    void add_param(ParameterList& param_list, const Parameter& param) const;
    void add_param_locator_seq(
        ParameterList& param_list,
        const LocatorSeq& locator_seq, 
        const ParameterId_t pid) const;
    void append_locator(LocatorSeq& list, const Locator_t& locator) const;
  };
}
}

#endif
