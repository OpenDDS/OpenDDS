/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "dds/DCPS/transport/framework/EntryExit.h"

ACE_INLINE
OpenDDS::DCPS::SimpleUdpConfiguration::SimpleUdpConfiguration()
{
  DBG_ENTRY_LVL("SimpleUdpConfiguration","SimpleUdpConfiguration",6);
  this->transport_type_ = ACE_TEXT("SimpleUdp");
}
