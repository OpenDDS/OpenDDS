/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DcpsInfo_pch.h"

#include /**/ "DCPS_IR_Subscription.h"

#include /**/ "DCPS_IR_Publication.h"
#include /**/ "DCPS_IR_Participant.h"
#include /**/ "DCPS_IR_Topic_Description.h"
#include /**/ "DCPS_IR_Domain.h"
#include /**/ "dds/DCPS/DCPS_Utils.h"
#include /**/ "dds/DCPS/RepoIdConverter.h"
#include /**/ "dds/DCPS/Qos_Helper.h"
#include /**/ "tao/debug.h"

#include /**/ "ace/OS_NS_unistd.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

DCPS_IR_Subscription::DCPS_IR_Subscription(const OpenDDS::DCPS::RepoId& id,
                                           DCPS_IR_Participant* participant,
                                           DCPS_IR_Topic* topic,
                                           OpenDDS::DCPS::DataReaderRemote_ptr reader,
                                           const DDS::DataReaderQos& qos,
                                           const OpenDDS::DCPS::TransportLocatorSeq& info,
                                           const DDS::SubscriberQos& subscriberQos,
                                           const char* filterClassName,
                                           const char* filterExpression,
                                           const DDS::StringSeq& exprParams)
  : id_(id),
    participant_(participant),
    topic_(topic),
    handle_(0),
    isBIT_(0),
    qos_(qos),
    info_(info),
    subscriberQos_(subscriberQos),
    filterClassName_(filterClassName),
    filterExpression_(filterExpression),
    exprParams_(exprParams)
{
  reader_ =  OpenDDS::DCPS::DataReaderRemote::_duplicate(reader);

  incompatibleQosStatus_.total_count = 0;
  incompatibleQosStatus_.count_since_last_send = 0;
}

DCPS_IR_Subscription::~DCPS_IR_Subscription()
{
  if (0 != associations_.size()) {
    CORBA::Boolean dont_notify_lost = 0;
    remove_associations(dont_notify_lost);
  }
}

int DCPS_IR_Subscription::add_associated_publication(DCPS_IR_Publication* pub,
                                                     bool active)
{
  // keep track of the association locally
  int status = associations_.insert(pub);

  switch (status) {
  case 0: {
    // inform the datareader about the association
    OpenDDS::DCPS::WriterAssociation association;
    association.writerTransInfo = pub->get_transportLocatorSeq();
    association.writerId = pub->get_id();
    association.pubQos = *(pub->get_publisher_qos());
    association.writerQos = *(pub->get_datawriter_qos());

    if (participant_->is_alive() && this->participant_->isOwner()) {
      try {
        if (OpenDDS::DCPS::DCPS_debug_level > 0) {
          OpenDDS::DCPS::RepoIdConverter sub_converter(id_);
          OpenDDS::DCPS::RepoIdConverter pub_converter(pub->get_id());
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) DCPS_IR_Subscription::add_associated_publication:")
                     ACE_TEXT(" subscription %C adding publication %C.\n"),
                     std::string(sub_converter).c_str(),
                     std::string(pub_converter).c_str()));
        }

        reader_->add_association(id_, association, active);

        if (OpenDDS::DCPS::DCPS_debug_level > 0) {
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) DCPS_IR_Subscription::add_associated_publication: ")
                     ACE_TEXT("successfully added publication %x\n"),
                     pub));
        }
      } catch (const CORBA::Exception& ex) {
        ex._tao_print_exception(
          "(%P|%t) ERROR: Exception caught in DCPS_IR_Subscription::add_associated_publication:");
        participant_->mark_dead();
        status = -1;
      }
    }

  }
  break;

  case 1: {
    OpenDDS::DCPS::RepoIdConverter sub_converter(id_);
    OpenDDS::DCPS::RepoIdConverter pub_converter(pub->get_id());
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Subscription::add_associated_publication: ")
               ACE_TEXT("subscription %C attempted to re-add publication %C\n"),
               std::string(sub_converter).c_str(),
               std::string(pub_converter).c_str()));
  }
  break;

  case -1: {
    OpenDDS::DCPS::RepoIdConverter sub_converter(id_);
    OpenDDS::DCPS::RepoIdConverter pub_converter(pub->get_id());
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Subscription::add_associated_publication: ")
               ACE_TEXT("subscription %C failed to add publication %C\n"),
               std::string(sub_converter).c_str(),
               std::string(pub_converter).c_str()));
  }
  };

  return status;
}

void
DCPS_IR_Subscription::association_complete(const OpenDDS::DCPS::RepoId& remote)
{
  typedef DCPS_IR_Publication_Set::ITERATOR iter_t;
  for (iter_t iter = associations_.begin(); iter != associations_.end(); ++iter) {
    if ((*iter)->get_id() == remote) {
      (*iter)->call_association_complete(get_id());
    }
  }
}

void
DCPS_IR_Subscription::call_association_complete(const OpenDDS::DCPS::RepoId& remote)
{
  try {
    reader_->association_complete(remote);
  } catch (const CORBA::Exception& ex) {
    ex._tao_print_exception(
      "(%P|%t) ERROR: Exception caught in DCPS_IR_Subscription::call_association_complete:");
    participant_->mark_dead();
  }
}

int DCPS_IR_Subscription::remove_associated_publication(DCPS_IR_Publication* pub,
                                                        CORBA::Boolean sendNotify,
                                                        CORBA::Boolean notify_lost,
                                                        bool notify_both_side)
{
  bool marked_dead = false;

  if (sendNotify) {

    if (participant_->is_alive() && this->participant_->isOwner()) {
      try {
        if (OpenDDS::DCPS::DCPS_debug_level > 0) {
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) DCPS_IR_Subscription::remove_associated_publication:")
                     ACE_TEXT(" calling sub %d with pub %d\n"),
                     id_, pub->get_id()));
        }

        OpenDDS::DCPS::WriterIdSeq idSeq(5);
        idSeq.length(1);
        idSeq[0] = pub->get_id();

        reader_->remove_associations(idSeq, notify_lost);

        if (notify_both_side) {
          pub->remove_associated_subscription(this, sendNotify, notify_lost);
        }

      } catch (const CORBA::Exception& ex) {
        if (OpenDDS::DCPS::DCPS_debug_level > 0) {
          ex._tao_print_exception(
            "(%P|%t) ERROR: Exception caught in DCPS_IR_Subscription::remove_associated_publication:");
        }

        participant_->mark_dead();
        marked_dead = true;
      }
    }
  }

  int status = associations_.remove(pub);

  if (0 == status) {
    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      OpenDDS::DCPS::RepoIdConverter sub_converter(id_);
      OpenDDS::DCPS::RepoIdConverter pub_converter(pub->get_id());
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DCPS_IR_Subscription::remove_associated_publication: ")
                 ACE_TEXT("subscription %C removed publication %C at %x.\n"),
                 std::string(sub_converter).c_str(),
                 std::string(pub_converter).c_str(),
                 pub));
    }

  } else {
    OpenDDS::DCPS::RepoIdConverter sub_converter(id_);
    OpenDDS::DCPS::RepoIdConverter pub_converter(pub->get_id());
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Subscription::remove_associated_publication: ")
               ACE_TEXT("subscription %C failed to remove publication %C at %x.\n"),
               std::string(sub_converter).c_str(),
               std::string(pub_converter).c_str(),
               pub));
  } // if (0 == status)

  if (marked_dead) {
    return -1;

  } else {
    return status;
  }
}

int DCPS_IR_Subscription::remove_associations(CORBA::Boolean notify_lost)
{
  int status = 0;
  DCPS_IR_Publication* pub = 0;
  size_t numAssociations = associations_.size();
  CORBA::Boolean dontSend = 0;
  CORBA::Boolean send = 1;

  if (0 < numAssociations) {
    DCPS_IR_Publication_Set::ITERATOR iter = associations_.begin();
    DCPS_IR_Publication_Set::ITERATOR end = associations_.end();

    while (iter != end) {
      pub = *iter;
      ++iter;

      pub->remove_associated_subscription(this, send, notify_lost);
      CORBA::Boolean dont_notify_lost = 0;
      remove_associated_publication(pub, dontSend, dont_notify_lost);
    }
  }
  this->defunct_.reset();

  return status;
}

void DCPS_IR_Subscription::disassociate_participant(OpenDDS::DCPS::RepoId id,
                                                    bool reassociate)
{
  DCPS_IR_Publication* pub = 0;
  size_t numAssociations = associations_.size();
  CORBA::Boolean dontSend = 0;
  CORBA::Boolean send = 1;
  long count = 0;

  if (0 < numAssociations) {
    OpenDDS::DCPS::WriterIdSeq idSeq(static_cast<CORBA::ULong>(numAssociations));
    idSeq.length(static_cast<CORBA::ULong>(numAssociations));

    DCPS_IR_Publication_Set::ITERATOR iter = associations_.begin();
    DCPS_IR_Publication_Set::ITERATOR end = associations_.end();

    while (iter != end) {
      pub = *iter;
      ++iter;

      if (OpenDDS::DCPS::DCPS_debug_level > 0) {
        OpenDDS::DCPS::RepoIdConverter sub_converter(id_);
        OpenDDS::DCPS::RepoIdConverter pub_converter(pub->get_id());
        OpenDDS::DCPS::RepoIdConverter sub_part_converter(id);
        OpenDDS::DCPS::RepoIdConverter pub_part_converter(pub->get_participant_id());
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) DCPS_IR_Subscription::disassociate_participant: ")
                   ACE_TEXT("subscription %C testing if publication %C particpant %C == %C.\n"),
                   std::string(sub_converter).c_str(),
                   std::string(pub_converter).c_str(),
                   std::string(sub_part_converter).c_str(),
                   std::string(pub_part_converter).c_str()));
      }

      if (id == pub->get_participant_id()) {
        CORBA::Boolean dont_notify_lost = 0;
        pub->remove_associated_subscription(this, send, dont_notify_lost);
        remove_associated_publication(pub, dontSend, dont_notify_lost);

        idSeq[count] = pub->get_id();
        ++count;

        if (reassociate && this->defunct_.insert(pub) != 0) {
          OpenDDS::DCPS::RepoIdConverter sub_converter(id_);
          OpenDDS::DCPS::RepoIdConverter pub_converter(pub->get_id());
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Subscription::disassociate_participant: ")
                     ACE_TEXT("subscription %C failed to reassociate publication %C at %x.\n"),
                     std::string(sub_converter).c_str(),
                     std::string(pub_converter).c_str(),
                     pub));
        }
      }
    }

    if (0 < count) {
      idSeq.length(count);

      if (participant_->is_alive() && this->participant_->isOwner()) {
        try {
          CORBA::Boolean dont_notify_lost = 0;
          reader_->remove_associations(idSeq, dont_notify_lost);

        } catch (const CORBA::Exception& ex) {
          if (OpenDDS::DCPS::DCPS_debug_level > 0) {
            ex._tao_print_exception(
              "(%P|%t) ERROR: Exception caught in DCPS_IR_Subscription::disassociate_participant:");
          }

          participant_->mark_dead();
        }
      }
    }
  }
}

void DCPS_IR_Subscription::disassociate_topic(OpenDDS::DCPS::RepoId id)
{
  DCPS_IR_Publication* pub = 0;
  size_t numAssociations = associations_.size();
  CORBA::Boolean dontSend = 0;
  CORBA::Boolean send = 1;
  long count = 0;

  if (0 < numAssociations) {
    OpenDDS::DCPS::WriterIdSeq idSeq(static_cast<CORBA::ULong>(numAssociations));
    idSeq.length(static_cast<CORBA::ULong>(numAssociations));

    DCPS_IR_Publication_Set::ITERATOR iter = associations_.begin();
    DCPS_IR_Publication_Set::ITERATOR end = associations_.end();

    while (iter != end) {
      pub = *iter;
      ++iter;

      if (OpenDDS::DCPS::DCPS_debug_level > 0) {
        OpenDDS::DCPS::RepoIdConverter sub_converter(id_);
        OpenDDS::DCPS::RepoIdConverter pub_converter(pub->get_id());
        OpenDDS::DCPS::RepoIdConverter sub_topic_converter(id);
        OpenDDS::DCPS::RepoIdConverter pub_topic_converter(pub->get_topic_id());
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) DCPS_IR_Subscription::disassociate_topic: ")
                   ACE_TEXT("subscription %C testing if publication %C topic %C == %C.\n"),
                   std::string(sub_converter).c_str(),
                   std::string(pub_converter).c_str(),
                   std::string(sub_topic_converter).c_str(),
                   std::string(pub_topic_converter).c_str()));
      }

      if (id == pub->get_topic_id()) {
        CORBA::Boolean dont_notify_lost = 0;
        pub->remove_associated_subscription(this, send, dont_notify_lost);
        remove_associated_publication(pub, dontSend, dont_notify_lost);

        idSeq[count] = pub->get_id();
        ++count;
      }
    }

    if (0 < count) {
      idSeq.length(count);

      if (participant_->is_alive() && this->participant_->isOwner()) {
        try {
          CORBA::Boolean dont_notify_lost = 0;
          reader_->remove_associations(idSeq, dont_notify_lost);

        } catch (const CORBA::Exception& ex) {
          if (OpenDDS::DCPS::DCPS_debug_level > 0) {
            ex._tao_print_exception(
              "(%P|%t) ERROR: Exception caught in DCPS_IR_Subscription::remove_associations:");
          }

          participant_->mark_dead();
        }
      }
    }
  }
}

void DCPS_IR_Subscription::disassociate_publication(OpenDDS::DCPS::RepoId id,
                                                    bool reassociate)
{
  DCPS_IR_Publication* pub = 0;
  size_t numAssociations = associations_.size();
  CORBA::Boolean dontSend = 0;
  CORBA::Boolean send = 1;
  long count = 0;

  if (0 < numAssociations) {
    OpenDDS::DCPS::WriterIdSeq idSeq(static_cast<CORBA::ULong>(numAssociations));
    idSeq.length(static_cast<CORBA::ULong>(numAssociations));

    DCPS_IR_Publication_Set::ITERATOR iter = associations_.begin();
    DCPS_IR_Publication_Set::ITERATOR end = associations_.end();

    while (iter != end) {
      pub = *iter;
      ++iter;

      if (OpenDDS::DCPS::DCPS_debug_level > 0) {
        OpenDDS::DCPS::RepoIdConverter sub_converter(id_);
        OpenDDS::DCPS::RepoIdConverter pub_converter(pub->get_id());
        OpenDDS::DCPS::RepoIdConverter sub_pub_converter(id);
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) DCPS_IR_Subscription::disassociate_publication: ")
                   ACE_TEXT("subscription %C testing if publication %C == %C.\n"),
                   std::string(sub_converter).c_str(),
                   std::string(pub_converter).c_str(),
                   std::string(sub_pub_converter).c_str()));
      }

      if (id == pub->get_id()) {
        CORBA::Boolean dont_notify_lost = 0;
        pub->remove_associated_subscription(this, send, dont_notify_lost);
        remove_associated_publication(pub, dontSend, dont_notify_lost);

        idSeq[count] = pub->get_id();
        ++count;

        if (reassociate && this->defunct_.insert(pub) != 0) {
          OpenDDS::DCPS::RepoIdConverter sub_converter(id_);
          OpenDDS::DCPS::RepoIdConverter pub_converter(pub->get_id());
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Subscription::disassociate_publication: ")
                     ACE_TEXT("subscription %C failed to reassociate publication %C at %x.\n"),
                     std::string(sub_converter).c_str(),
                     std::string(pub_converter).c_str(),
                     pub));
        }
      }
    }

    if (0 < count) {
      idSeq.length(count);

      if (participant_->is_alive() && this->participant_->isOwner()) {
        try {
          CORBA::Boolean dont_notify_lost = 0;
          reader_->remove_associations(idSeq, dont_notify_lost);

        } catch (const CORBA::Exception& ex) {
          if (OpenDDS::DCPS::DCPS_debug_level > 0) {
            ex._tao_print_exception(
              "(%P|%t) ERROR: Exception caught in DCPS_IR_Subscription::remove_associations:");
          }

          participant_->mark_dead();
        }
      }
    }
  }
}

void DCPS_IR_Subscription::update_incompatible_qos()
{
  if (participant_->is_alive() && this->participant_->isOwner()) {
    try {
      reader_->update_incompatible_qos(incompatibleQosStatus_);
      incompatibleQosStatus_.count_since_last_send = 0;
    } catch (const CORBA::Exception& ex) {
      if (OpenDDS::DCPS::DCPS_debug_level > 0) {
        ex._tao_print_exception(
          "(%P|%t) ERROR: Exception caught in DCPS_IR_Subscription::update_incompatible_qos:");
      }

      participant_->mark_dead();
    }
  }
}

CORBA::Boolean DCPS_IR_Subscription::is_publication_ignored(OpenDDS::DCPS::RepoId partId,
                                                            OpenDDS::DCPS::RepoId topicId,
                                                            OpenDDS::DCPS::RepoId pubId)
{
  CORBA::Boolean ignored;
  ignored = (participant_->is_participant_ignored(partId) ||
             participant_->is_topic_ignored(topicId) ||
             participant_->is_publication_ignored(pubId));

  return ignored;
}

OpenDDS::DCPS::TransportLocatorSeq DCPS_IR_Subscription::get_transportLocatorSeq() const
{
  return info_;
}

OpenDDS::DCPS::IncompatibleQosStatus* DCPS_IR_Subscription::get_incompatibleQosStatus()
{
  return &incompatibleQosStatus_;
}

const DDS::DataReaderQos* DCPS_IR_Subscription::get_datareader_qos()
{
  return &qos_;
}

const DDS::SubscriberQos* DCPS_IR_Subscription::get_subscriber_qos()
{
  return &subscriberQos_;
}

using OpenDDS::DCPS::operator==;

void
DCPS_IR_Subscription::set_qos(const DDS::DataReaderQos& qos)
{
  if (false == (qos == this->qos_)) {
    // Check if we should check while we have both values.
    bool check =
      OpenDDS::DCPS::should_check_association_upon_change(qos, this->qos_);

    // Store the new, compatible, value.
    this->qos_ = qos;

    if (check) {
      // This will remove any newly stale associations.
      this->reevaluate_existing_associations();

      // Sleep a while to let remove_association handled by DataWriter
      // before add_association. Otherwise, new association will have
      // trouble to connect each other.
      ACE_OS::sleep(ACE_Time_Value(0, 250000));

      // This will establish any newly made associations.
      DCPS_IR_Topic_Description* description
      = this->topic_->get_topic_description();
      description->reevaluate_associations(this);
    }

    this->participant_->get_domain_reference()->publish_subscription_bit(this);
  }
}

void
DCPS_IR_Subscription::set_qos(const DDS::SubscriberQos& qos)
{
  if (false == (qos == this->subscriberQos_)) {
    // Check if we should check while we have both values.
    bool check = OpenDDS::DCPS::should_check_association_upon_change(qos, this->subscriberQos_);

    // Store the new, compatible, value.
    this->subscriberQos_ = qos;

    if (check) {
      // This will remove any newly stale associations.
      this->reevaluate_existing_associations();

      // Sleep a while to let remove_association handled by DataWriter
      // before add_association. Otherwise, new association will have
      // trouble to connect each other.
      ACE_OS::sleep(ACE_Time_Value(0, 250000));

      // This will establish any newly made associations.
      DCPS_IR_Topic_Description* description
      = this->topic_->get_topic_description();
      description->reevaluate_associations(this);
    }

    this->participant_->get_domain_reference()->publish_subscription_bit(this);
  }
}

bool DCPS_IR_Subscription::set_qos(const DDS::DataReaderQos & qos,
                                   const DDS::SubscriberQos & subscriberQos,
                                   Update::SpecificQos& specificQos)
{
  bool need_evaluate = false;
  bool u_dr_qos = !(qos_ == qos);

  if (u_dr_qos) {
    if (OpenDDS::DCPS::should_check_association_upon_change(qos_, qos)) {
      need_evaluate = true;
    }

    qos_ = qos;
  }

  bool u_sub_qos = !(subscriberQos_ == subscriberQos);

  if (u_sub_qos) {
    if (OpenDDS::DCPS::should_check_association_upon_change(subscriberQos_, subscriberQos)) {
      need_evaluate = true;
    }

    subscriberQos_ = subscriberQos;
  }

  if (need_evaluate) {
    // Check if any existing association need be removed first.
    this->reevaluate_existing_associations();

    DCPS_IR_Topic_Description* description = this->topic_->get_topic_description();
    description->reevaluate_associations(this);
  }

  participant_->get_domain_reference()->publish_subscription_bit(this);
  specificQos = u_dr_qos?  Update::DataReaderQos:
                u_sub_qos? Update::SubscriberQos:
                Update::NoQos;

  return true;
}

void
DCPS_IR_Subscription::reevaluate_defunct_associations()
{
  DCPS_IR_Publication_Set::iterator it(this->defunct_.begin());
  while (it != this->defunct_.end()) {
    DCPS_IR_Publication* publication = *it;
    ++it;

    if (reevaluate_association(publication)) {
      this->defunct_.remove(publication); // no longer defunct

    } else {
      OpenDDS::DCPS::RepoIdConverter sub_converter(id_);
      OpenDDS::DCPS::RepoIdConverter pub_converter(publication->get_id());
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Subscription::reevaluate_defunct_associations: ")
                 ACE_TEXT("subscription %C failed to reassociate publication %C at %x.\n"),
                 std::string(sub_converter).c_str(),
                 std::string(pub_converter).c_str(),
                 publication));
    }
  }
}

void DCPS_IR_Subscription::reevaluate_existing_associations()
{
  DCPS_IR_Publication * pub = 0;
  DCPS_IR_Publication_Set::ITERATOR iter = associations_.begin();
  DCPS_IR_Publication_Set::ITERATOR end = associations_.end();

  while (iter != end) {
    pub = *iter;
    ++iter;

    this->reevaluate_association(pub);
  }
}

bool
DCPS_IR_Subscription::reevaluate_association(DCPS_IR_Publication* publication)
{
  int status = this->associations_.find(publication);

  if (status == 0) {
    // verify if they are still compatiable after change

    if (!OpenDDS::DCPS::compatibleQOS(publication->get_incompatibleQosStatus(),
                                      this->get_incompatibleQosStatus(),
                                      publication->get_transportLocatorSeq(),
                                      this->get_transportLocatorSeq(),
                                      publication->get_datawriter_qos(),
                                      this->get_datareader_qos(),
                                      publication->get_publisher_qos(),
                                      this->get_subscriber_qos())) {
      bool sendNotify = true; // inform datareader
      bool notify_lost = true; // invoke listerner callback

      this->remove_associated_publication(publication, sendNotify, notify_lost, true);
    }

  } else {
    DCPS_IR_Topic_Description* description = this->topic_->get_topic_description();
    return description->try_associate(publication, this);
  }

  return false;
}

OpenDDS::DCPS::RepoId DCPS_IR_Subscription::get_id()
{
  return id_;
}

OpenDDS::DCPS::RepoId DCPS_IR_Subscription::get_topic_id()
{
  return topic_->get_id();
}

OpenDDS::DCPS::RepoId DCPS_IR_Subscription::get_participant_id()
{
  return participant_->get_id();
}

DCPS_IR_Topic_Description* DCPS_IR_Subscription::get_topic_description()
{
  return topic_->get_topic_description();
}

DCPS_IR_Topic* DCPS_IR_Subscription::get_topic()
{
  return topic_;
}

DDS::InstanceHandle_t DCPS_IR_Subscription::get_handle()
{
  return handle_;
}

void DCPS_IR_Subscription::set_handle(DDS::InstanceHandle_t handle)
{
  handle_ = handle;
}

CORBA::Boolean DCPS_IR_Subscription::is_bit()
{
  return isBIT_;
}

void DCPS_IR_Subscription::set_bit_status(CORBA::Boolean isBIT)
{
  isBIT_ = isBIT;
}

OpenDDS::DCPS::DataReaderRemote_ptr
DCPS_IR_Subscription::reader()
{
  return OpenDDS::DCPS::DataReaderRemote::_duplicate(this->reader_.in());
}

std::string
DCPS_IR_Subscription::get_filter_class_name() const
{
  return filterClassName_;
}

std::string
DCPS_IR_Subscription::get_filter_expression() const
{
  return filterExpression_;
}

DDS::StringSeq
DCPS_IR_Subscription::get_expr_params() const
{
  return exprParams_;
}

void
DCPS_IR_Subscription::update_expr_params(const DDS::StringSeq& params)
{
  exprParams_ = params;
  typedef DCPS_IR_Publication_Set::ITERATOR iter_t;
  for (iter_t i(associations_.begin()), e(associations_.end()); i != e; ++i) {
    (*i)->update_expr_params(id_, params);
  }
}

std::string
DCPS_IR_Subscription::dump_to_string(const std::string& prefix, int depth) const
{
  std::string str;
#if !defined (OPENDDS_INFOREPO_REDUCED_FOOTPRINT)
  OpenDDS::DCPS::RepoIdConverter local_converter(id_);

  for (int i=0; i < depth; i++)
    str += prefix;
  std::string indent = str + prefix;
  str += "DCPS_IR_Subscription[";
  str += std::string(local_converter);
  str += "]";
  if (isBIT_)
    str += " (BIT)";
  str += "\n";

  str += indent + "Associations [ ";
  for (DCPS_IR_Publication_Set::const_iterator assoc = associations_.begin();
       assoc != associations_.end();
       assoc++)
  {
    OpenDDS::DCPS::RepoIdConverter assoc_converter((*assoc)->get_id());
    str += std::string(assoc_converter);
    str += " ";
  }
  str += "]\n";

  str += indent + "Defunct Associations [ ";
  for (DCPS_IR_Publication_Set::const_iterator def = defunct_.begin();
       def != defunct_.end();
       def++)
  {
    OpenDDS::DCPS::RepoIdConverter def_converter((*def)->get_id());
    str += std::string(def_converter);
    str += " ";
  }
  str += "]\n";

#endif // !defined (OPENDDS_INFOREPO_REDUCED_FOOTPRINT)
  return str;
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
