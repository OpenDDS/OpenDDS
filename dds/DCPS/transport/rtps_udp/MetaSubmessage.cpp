/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MetaSubmessage.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

bool MetaSubmessage::operator==(const MetaSubmessage& other) const
{
  ACE_Message_Block left_block(1024);
  Serializer left(&left_block, Encoding::KIND_XCDR2);
  left << sm_;

  ACE_Message_Block right_block(1024);
  Serializer right(&right_block, Encoding::KIND_XCDR2);
  right << sm_;

  return src_guid_ == other.src_guid_ &&
    dst_guid_ == other.dst_guid_ &&
    left_block.length() == right_block.length() &&
    std::memcmp(left_block.rd_ptr(), right_block.rd_ptr(), left_block.length()) == 0 &&
    ignore_ == other.ignore_;
}

namespace {
  struct SortPredicate {
    bool operator()(const MetaSubmessage& x, const MetaSubmessage& y) const
    {
      if (x.src_guid_ != y.src_guid_) {
        return x.src_guid_ < y.src_guid_;
      }

      if (x.dst_guid_ != y.dst_guid_) {
        return x.dst_guid_ < y.dst_guid_;
      }

      if (x.sm_._d() != y.sm_._d()) {
        return x.sm_._d() < y.sm_._d();
      }

      // Order heartbeats and acknacks by count, descending.
      switch (x.sm_._d()) {
      case RTPS::HEARTBEAT:
        return x.sm_.heartbeat_sm().count.value > y.sm_.heartbeat_sm().count.value;
        break;
      case RTPS::ACKNACK:
        return x.sm_.acknack_sm().count.value > y.sm_.acknack_sm().count.value;
        break;
      default:
        return false;
      }
    }
  };

  struct EqualPredicate {
    bool operator()(const MetaSubmessage& x, const MetaSubmessage& y) const
    {
      return x.src_guid_ == y.src_guid_ &&
        x.dst_guid_ == y.dst_guid_ &&
        x.sm_._d() == y.sm_._d();
    }
  };
}

size_t dedup(MetaSubmessageVec& vec)
{
  size_t count = 0;

  if (vec.empty()) {
    return count;
  }

  std::sort(vec.begin(), vec.end(), SortPredicate());
  MetaSubmessageVec::iterator pos = vec.begin();
  MetaSubmessageVec::iterator next = pos;
  std::advance(next, 1);
  const MetaSubmessageVec::iterator limit = vec.end();
  EqualPredicate eq;
  for (; next != limit; ++pos, ++next) {
    if ((pos->sm_._d() == RTPS::HEARTBEAT || pos->sm_._d() == RTPS::ACKNACK) && eq(*pos, *next)) {
      next->ignore_ = true;
      ++count;
    }
  }

  return count;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
