/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORTCUSTOMIZEDELEMENT_H
#define OPENDDS_DCPS_TRANSPORTCUSTOMIZEDELEMENT_H

#include "dds/DCPS/dcps_export.h"
#include "TransportSendElement.h"

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export TransportCustomizedElement
  : public TransportQueueElement {

public:
  static TransportCustomizedElement* alloc(TransportSendElement* tse);

  virtual RepoId publication_id() const;

  virtual const ACE_Message_Block* msg() const;
  void set_msg(ACE_Message_Block* m);

  virtual bool owned_by_transport();

protected:
  virtual void release_element(bool dropped_by_transport);

private:
  TransportCustomizedElement(TransportSendElement* tse,
                             ACE_Allocator* allocator);
  virtual ~TransportCustomizedElement();

  TransportSendElement* tse_;
  ACE_Message_Block* msg_;
  ACE_Allocator* allocator_;
};

} // namespace DCPS
} // namespace OpenDDS

#if defined (__ACE_INLINE__)
#include "TransportCustomizedElement.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_TRANSPORTCUSTOMIZEDELEMENT_H */
