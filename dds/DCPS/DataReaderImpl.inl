/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

ACE_INLINE
void
OpenDDS::DCPS::DataReaderImpl::disable_transport()
{
  this->transport_disabled_ = true;
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
