/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_FRAMEWORK_TRANSPORTCONTROLELEMENT_H
#define OPENDDS_DCPS_TRANSPORT_FRAMEWORK_TRANSPORTCONTROLELEMENT_H

#include "dds/DCPS/dcps_export.h"
#include "dds/DCPS/GuidUtils.h"
#include "dds/DCPS/Message_Block_Ptr.h"
#include "TransportDefs.h"
#include "TransportQueueElement.h"

ACE_BEGIN_VERSIONED_NAMESPACE_DECL
class ACE_Message_Block;
ACE_END_VERSIONED_NAMESPACE_DECL

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export TransportControlElement
  : public TransportQueueElement {
public:

  /**
   * msg_block - chain of ACE_Message_Blocks containing the control
   *             sample held by this queue element, if any.
   */
  explicit TransportControlElement(Message_Block_Ptr msg_block);

  virtual ~TransportControlElement();

  virtual bool owned_by_transport();

protected:

  virtual bool requires_exclusive_packet() const;

  virtual RepoId publication_id() const;

  virtual ACE_Message_Block* duplicate_msg() const;

  virtual const ACE_Message_Block* msg() const;

  virtual const ACE_Message_Block* msg_payload() const;

  virtual void release_element(bool dropped_by_transport);

  virtual void data_delivered();

private:
  TransportControlElement(const TransportControlElement&);
  /// The control message.
  Message_Block_Ptr msg_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
#include "TransportControlElement.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_TRANSPORTGDCONTROLELEMENT_H */
