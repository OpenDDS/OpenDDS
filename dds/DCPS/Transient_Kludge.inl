/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

ACE_INLINE
OpenDDS::DCPS::Transient_Kludge::Transient_Kludge()
  : enabled_(false)
{
}

ACE_INLINE
OpenDDS::DCPS::Transient_Kludge::~Transient_Kludge()
{
}

ACE_INLINE
void
OpenDDS::DCPS::Transient_Kludge::enable()
{
  enabled_ = true;
}

ACE_INLINE
void
OpenDDS::DCPS::Transient_Kludge::disable()
{
  enabled_ = false;
}

ACE_INLINE
bool
OpenDDS::DCPS::Transient_Kludge::is_enabled()
{
  return enabled_;
}
