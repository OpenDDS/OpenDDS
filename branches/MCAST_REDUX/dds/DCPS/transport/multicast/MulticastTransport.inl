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

ACE_INLINE MulticastConfiguration*
MulticastTransport::get_configuration()
{
  return this->config_i_.in();
}

} // namespace DCPS
} // namespace OpenDDS
