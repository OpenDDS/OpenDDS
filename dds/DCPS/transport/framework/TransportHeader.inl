/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "dds/DCPS/Serializer.h"
#include "EntryExit.h"

#include <algorithm>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

ACE_INLINE
TransportHeader::TransportHeader()
  : byte_order_(ACE_CDR_BYTE_ORDER),
    first_fragment_(false),
    last_fragment_(false),
    reserved_(0),
    length_(0),
    sequence_(),
    source_(0)
{
  DBG_ENTRY_LVL("TransportHeader","TransportHeader",6);

  std::copy(&DCPS_PROTOCOL[0], &DCPS_PROTOCOL[6], this->protocol_);
}

ACE_INLINE
TransportHeader::TransportHeader(const TransportHeader::no_init_t&)
{
}

ACE_INLINE
TransportHeader::TransportHeader(ACE_Message_Block& buffer)
{
  DBG_ENTRY_LVL("TransportHeader","TransportHeader",6);
  this->init(&buffer);
}

ACE_INLINE
TransportHeader&
TransportHeader::operator=(ACE_Message_Block& buffer)
{
  DBG_ENTRY_LVL("TransportHeader","operator=",6);
  this->init(&buffer);
  return *this;
}

ACE_INLINE
size_t
TransportHeader::max_marshaled_size()
{
  // Representation takes no extra space for encoding.
  TransportHeader hdr(no_init);
  return sizeof(hdr.protocol_) +
         1 /*flags*/ +
         sizeof(hdr.reserved_) +
         sizeof(hdr.length_) +
         sizeof(hdr.sequence_) +
         sizeof(hdr.source_);
}

ACE_INLINE
bool
TransportHeader::swap_bytes() const
{
  DBG_ENTRY_LVL("TransportHeader","swap_bytes",6);

  return this->byte_order_ != ACE_CDR_BYTE_ORDER;
}

ACE_INLINE
bool
TransportHeader::valid() const
{
  DBG_ENTRY_LVL("TransportHeader","valid",6);

  // Currently we do not support compatibility with other
  // versions of the protocol:
  return std::equal(&DCPS_PROTOCOL[0], &DCPS_PROTOCOL[6], this->protocol_);
}

ACE_INLINE
void
TransportHeader::init(ACE_Message_Block* buffer)
{
  DBG_ENTRY_LVL("TransportHeader","init",6);

  Serializer reader(buffer);

  reader.read_octet_array(this->protocol_, sizeof(this->protocol_));

  ACE_CDR::Octet flags;
  reader >> ACE_InputCDR::to_octet(flags);
  this->byte_order_= flags & (1 << BYTE_ORDER_FLAG);
  this->first_fragment_ = flags & (1 << FIRST_FRAGMENT_FLAG);
  this->last_fragment_ = flags & (1 << LAST_FRAGMENT_FLAG);

  reader >> ACE_InputCDR::to_octet(this->reserved_);

  reader.swap_bytes(swap_bytes());

  reader >> this->length_;

  reader >> this->sequence_;

  reader >> this->source_;
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
