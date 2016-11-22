/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "Replayer.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {
ReplayerListener::~ReplayerListener()
{
}

void ReplayerListener::on_replayer_matched(Replayer*                               replayer,
                                           const ::DDS::PublicationMatchedStatus & status )
{
  ACE_UNUSED_ARG(replayer);
  ACE_UNUSED_ARG(status);
}

Replayer::~Replayer()
{
}

Replayer_ptr Replayer::_duplicate(Replayer_ptr obj)
{
  if (obj) obj->_add_ref();
  return obj;
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

OpenDDS::DCPS::Replayer_ptr
TAO::Objref_Traits<OpenDDS::DCPS::Replayer>::duplicate(OpenDDS::DCPS::Replayer_ptr p)
{
  return OpenDDS::DCPS::Replayer::_duplicate(p);
}

void
TAO::Objref_Traits<OpenDDS::DCPS::Replayer>::release(OpenDDS::DCPS::Replayer_ptr p)
{
  if (p) p->_remove_ref();
}

OpenDDS::DCPS::Replayer_ptr
TAO::Objref_Traits<OpenDDS::DCPS::Replayer>::nil()
{
  return static_cast<OpenDDS::DCPS::Replayer_ptr>(0);
}

CORBA::Boolean
TAO::Objref_Traits<OpenDDS::DCPS::Replayer>::marshal(const OpenDDS::DCPS::Replayer_ptr,
                                                     TAO_OutputCDR&)
{
  return false;
}

