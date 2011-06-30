/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "TransportConfig.h"
#include "TransportInst.h"

#if !defined (__ACE_INLINE__)
# include "TransportConfig.inl"
#endif /* !__ACE_INLINE__ */

namespace OpenDDS {
namespace DCPS {

TransportConfig::TransportConfig(const std::string& name)
: name_(name)
{}

TransportConfig::~TransportConfig()
{}

}
}
