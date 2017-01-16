/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
#ifndef OPENDDS_DCPS_DATASAMPLEELEMENT_H
#define OPENDDS_DCPS_DATASAMPLEELEMENT_H

#include "dds/DdsDcpsInfoUtilsC.h"
#include "Definitions.h"
#include "transport/framework/TransportDefs.h"
#include "Dynamic_Cached_Allocator_With_Overflow_T.h"
#include "DataSampleHeader.h"
#include "dds/DCPS/PoolAllocator.h"
#include "dds/DCPS/PoolAllocationBase.h"
#include "dds/DCPS/RcHandle_T.h"

class DDS_TEST;

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

const CORBA::ULong MAX_READERS_PER_ELEM = 5;

class DataSampleElement;
typedef Cached_Allocator_With_Overflow<DataSampleElement, ACE_Null_Mutex>
  DataSampleElementAllocator;

typedef Dynamic_Cached_Allocator_With_Overflow<ACE_Thread_Mutex>
  TransportSendElementAllocator;

class TransportCustomizedElement;
typedef Dynamic_Cached_Allocator_With_Overflow<ACE_Thread_Mutex>
  TransportCustomizedElementAllocator;

class TransportSendListener;
struct PublicationInstance;
typedef RcHandle<PublicationInstance> PublicationInstance_rch;

/**
* Currently we contain entire messages in a single ACE_Message_Block
* chain.
*/
typedef ACE_Message_Block DataSample;

/**
* Wraps the marshaled message sample to be published, along with
* the publication Id and Instance handle for downstream processing.
*
* Internally there are next/previous pointers that used for lists
* InstanceDataSampleList, SendStateDataSampleList, and WriterDataSampleList.
* These pointers are kept in this single element rather than having multiple smaller
* lists in order to allow us to allocate once which will minimize locking.
* Note that because the list pointers are stored within the element,
* the element can simultaneously be in at most one InstanceDataSampleList list, one
* SendStateDataSampleList list, and one WriterDataSampleList list.
*/
class OpenDDS_Dcps_Export DataSampleElement : public PoolAllocationBase {


public:
  DataSampleElement(PublicationId                   publication_id,
                    TransportSendListener*          send_listener,
                    PublicationInstance_rch         handle,
                    TransportSendElementAllocator*  tse_allocator,
                    TransportCustomizedElementAllocator* tce_allocator);

  DataSampleElement(const DataSampleElement& elem);
  DataSampleElement& operator=(const DataSampleElement& elem);

  ~DataSampleElement();

  const DataSampleHeader& get_header() const;
  DataSampleHeader& get_header();

  DataSample* get_sample() const;
  DataSample* get_sample();

  void set_sample(DataSample* sample);

  PublicationId get_pub_id() const;

  CORBA::ULong get_num_subs() const;

  void set_num_subs(int num_subs);

  const OpenDDS::DCPS::RepoId* get_sub_ids() const;
  OpenDDS::DCPS::RepoId  get_sub_id(int index) const;

  void set_sub_id(int index, OpenDDS::DCPS::RepoId id);

  TransportSendListener* get_send_listener() const;
  TransportSendListener* get_send_listener();

  PublicationInstance_rch get_handle() const;

  TransportSendElementAllocator* get_transport_send_element_allocator() const;

  TransportCustomizedElementAllocator* get_transport_customized_element_allocator() const;

  typedef OPENDDS_MAP(DataLinkIdType, GUIDSeq_var) DataLinkIdTypeGUIDMap;
  DataLinkIdTypeGUIDMap& get_filter_per_link();

  void set_filter_out(GUIDSeq *filter_out);

  void set_transaction_id(ACE_UINT64 transaction_id);

  ACE_UINT64 transaction_id() const;

private:

  ACE_UINT64 transaction_id_;

  /// The OpenDDS DCPS header for this sample
  DataSampleHeader       header_;

  /// Message being sent which includes the DataSampleHeader message block
  /// and DataSample message block.
  DataSample*            sample_;

  /// Publication Id used downstream.
  PublicationId          publication_id_;

  CORBA::ULong           num_subs_;

  OpenDDS::DCPS::RepoId  subscription_ids_[OpenDDS::DCPS::MAX_READERS_PER_ELEM];

  /// Pointer to object that will be informed when the data has
  /// been delivered.  This needs to be set prior to using the
  /// TransportClient to send().
  TransportSendListener* send_listener_;

  /// The pointer to the object that contains the instance information
  /// and data sample list.
  /// The client holds this as an InstanceHandle_t.
  PublicationInstance_rch   handle_;

  /// Allocator for the TransportSendElement.
  TransportSendElementAllocator* transport_send_element_allocator_;

  /// Allocator for TransportCustomizedElement
  TransportCustomizedElementAllocator* transport_customized_element_allocator_;

  //{@
  /// tracking for Content-Filtering data
  GUIDSeq_var filter_out_;
  DataLinkIdTypeGUIDMap filter_per_link_;
  //@}

  DataSampleElement* get_next_send_sample() const;

  void set_next_send_sample(DataSampleElement* next_send_sample);

  /// *DataSampleList(s) is in charge of managing list placement therefore
  /// needs access to pointers
  friend class SendStateDataSampleList;
  friend class WriterDataSampleList;
  friend class InstanceDataSampleList;
  friend class TransportClient;
  friend class ::DDS_TEST;
  /// Iterators needs access to prev/next pointers for iteration
  friend class SendStateDataSampleListIterator;
  friend class SendStateDataSampleListConstIterator;


  /// Used to make removal from the
  /// container _much_ more efficient.

  /// Thread of all data within a DataWriter.
  mutable DataSampleElement* previous_writer_sample_;
  mutable DataSampleElement* next_writer_sample_;

  /// Thread of data within the instance.
  mutable DataSampleElement* next_instance_sample_;
  mutable DataSampleElement* previous_instance_sample_;

  /// Thread of data being unsent/sending/sent/released.
  mutable DataSampleElement* next_send_sample_;
  mutable DataSampleElement* previous_send_sample_;
};


} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined(__ACE_INLINE__)
#include "DataSampleElement.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_DATASAMPLEELEMENT_H */
