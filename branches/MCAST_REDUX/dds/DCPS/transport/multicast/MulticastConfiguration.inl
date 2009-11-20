/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

namespace OpenDDS {
namespace DCPS {

ACE_INLINE ACE_Time_Value
MulticastConfiguration::syn_interval() const
{
  ACE_Time_Value tv;
  tv.msec(this->syn_interval_);
  return tv;
}

ACE_INLINE ACE_Time_Value
MulticastConfiguration::syn_timeout() const
{
  ACE_Time_Value tv;
  tv.msec(this->syn_timeout_);
  return tv;
}

} // namespace DCPS
} // namespace OpenDDS
