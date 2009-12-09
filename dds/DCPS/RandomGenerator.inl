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

ACE_INLINE RandomGenerator&
RandomGenerator::operator++()
{
  this->value_ = static_cast<double>(std::rand()) /
                 static_cast<double>(RAND_MAX);
  return *this;
}

ACE_INLINE RandomGenerator
RandomGenerator::operator++(int)
{
  RandomGenerator prev(*this);
  ++*this;
  return prev;
}

ACE_INLINE
RandomGenerator::operator double() const
{
  return this->value_;
}

} // namespace DCPS
} // namespace OpenDDS
