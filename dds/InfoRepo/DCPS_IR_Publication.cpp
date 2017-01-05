/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DcpsInfo_pch.h"
#include /**/ "DCPS_IR_Publication.h"

#include /**/ "DCPS_IR_Participant.h"
#include /**/ "DCPS_IR_Topic.h"
#include /**/ "DCPS_IR_Subscription.h"
#include /**/ "DCPS_IR_Domain.h"
#include /**/ "DCPS_IR_Topic_Description.h"
#include /**/ "dds/DCPS/DCPS_Utils.h"
#include /**/ "dds/DdsDcpsInfoUtilsC.h"
#include /**/ "dds/DCPS/RepoIdConverter.h"
#include /**/ "dds/DCPS/Qos_Helper.h"
#include /**/ "tao/debug.h"

#include /**/ "ace/OS_NS_unistd.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

DCPS_IR_Publication::DCPS_IR_Publication(const OpenDDS::DCPS::RepoId& id,
                                         DCPS_IR_Participant* participant,
                                         DCPS_IR_Topic* topic,
                                         OpenDDS::DCPS::DataWriterRemote_ptr writer,
                                         const DDS::DataWriterQos& qos,
                                         const OpenDDS::DCPS::TransportLocatorSeq& info,
                                         const DDS::PublisherQos& publisherQos)
  : id_(id),
    participant_(participant),
    topic_(topic),
    handle_(0),
    isBIT_(0),
    qos_(qos),
    info_(info),
    publisherQos_(publisherQos)
{
  writer_ =  OpenDDS::DCPS::DataWriterRemote::_duplicate(writer);

  incompatibleQosStatus_.total_count = 0;
  incompatibleQosStatus_.count_since_last_send = 0;
}

DCPS_IR_Publication::~DCPS_IR_Publication()
{
  if (0 != associations_.size()) {
    CORBA::Boolean dont_notify_lost = 0;
    remove_associations(dont_notify_lost);
  }
}

int DCPS_IR_Publication::add_associated_subscription(DCPS_IR_Subscription* sub,
                                                     bool active)
{
  // keep track of the association locally
  int status = associations_.insert(sub);

  switch (status) {
  case 0: {
    // inform the datawriter about the association
    OpenDDS::DCPS::ReaderAssociation association;
    association.readerTransInfo = sub->get_transportLocatorSeq();
    association.readerId = sub->get_id();
    association.subQos = *(sub->get_subscriber_qos());
    association.readerQos = *(sub->get_datareader_qos());
    association.filterClassName = sub->get_filter_class_name().c_str();
    association.filterExpression = sub->get_filter_expression().c_str();
    association.exprParams = sub->get_expr_params();

    if (participant_->is_alive() && this->participant_->isOwner()) {
      try {
        if (OpenDDS::DCPS::DCPS_debug_level > 0) {
          OpenDDS::DCPS::RepoIdConverter pub_converter(id_);
          OpenDDS::DCPS::RepoIdConverter sub_converter(sub->get_id());
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) DCPS_IR_Publication::add_associated_subscription:")
                     ACE_TEXT(" publication %C adding subscription %C.\n"),
                     std::string(pub_converter).c_str(),
                     std::string(sub_converter).c_str()));
        }

        writer_->add_association(id_, association, active);

        if (OpenDDS::DCPS::DCPS_debug_level > 0) {
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) DCPS_IR_Publication::add_associated_subscription: ")
                     ACE_TEXT("successfully added subscription %x.\n"),
                     sub));
        }
      } catch (const CORBA::Exception& ex) {
        ex._tao_print_exception(
          "(%P|%t) ERROR: Exception caught in DCPS_IR_Publication::add_associated_subscription:");
        participant_->mark_dead();
        status = -1;
      }
    }
  }
  break;

  case 1: {
    OpenDDS::DCPS::RepoIdConverter pub_converter(id_);
    OpenDDS::DCPS::RepoIdConverter sub_converter(sub->get_id());
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Publication::add_associated_subscription: ")
               ACE_TEXT("publication %C attempted to re-add subscription %C.\n"),
               std::string(pub_converter).c_str(),
               std::string(sub_converter).c_str()));
  }
  break;

  case -1: {
    OpenDDS::DCPS::RepoIdConverter pub_converter(id_);
    OpenDDS::DCPS::RepoIdConverter sub_converter(sub->get_id());
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Publication::add_associated_subscription: ")
               ACE_TEXT("publication %C failed to add subscription %C.\n"),
               std::string(pub_converter).c_str(),
               std::string(sub_converter).c_str()));
  }
  };

  return status;
}

void
DCPS_IR_Publication::association_complete(const OpenDDS::DCPS::RepoId& remote)
{
  typedef DCPS_IR_Subscription_Set::ITERATOR iter_t;
  for (iter_t iter = associations_.begin(); iter != associations_.end(); ++iter) {
    if ((*iter)->get_id() == remote) {
      (*iter)->call_association_complete(get_id());
    }
  }
}

void
DCPS_IR_Publication::call_association_complete(const OpenDDS::DCPS::RepoId& remote)
{
  try {
    writer_->association_complete(remote);
  } catch (const CORBA::Exception& ex) {
    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      ex._tao_print_exception(
        "(%P|%t) ERROR: Exception caught in DCPS_IR_Publication::call_association_complete:");
    }
    participant_->mark_dead();
  }
}

int DCPS_IR_Publication::remove_associated_subscription(DCPS_IR_Subscription* sub,
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
                     ACE_TEXT("(%P|%t) DCPS_IR_Publication::remove_associated_subscription:")
                     ACE_TEXT(" calling pub %d with sub %d\n"),
                     id_, sub->get_id()));
        }

        OpenDDS::DCPS::ReaderIdSeq idSeq(1);
        idSeq.length(1);
        idSeq[0] = sub->get_id();

        writer_->remove_associations(idSeq, notify_lost);

        if (notify_both_side) {
          sub->remove_associated_publication(this, sendNotify, notify_lost);
        }

      } catch (const CORBA::Exception& ex) {
        if (OpenDDS::DCPS::DCPS_debug_level > 0) {
          ex._tao_print_exception(
            "(%P|%t) ERROR: Exception caught in DCPS_IR_Publication::remove_associated_subscription:");
        }

        participant_->mark_dead();
        marked_dead = true;
      }
    }
  }

  int status = associations_.remove(sub);

  if (0 == status) {
    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      OpenDDS::DCPS::RepoIdConverter pub_converter(id_);
      OpenDDS::DCPS::RepoIdConverter sub_converter(sub->get_id());
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DCPS_IR_Publication::remove_associated_subscription: ")
                 ACE_TEXT("publication %C removed subscription %C at %x.\n"),
                 std::string(pub_converter).c_str(),
                 std::string(sub_converter).c_str(),
                 sub));
    }

  } else {
    OpenDDS::DCPS::RepoIdConverter pub_converter(id_);
    OpenDDS::DCPS::RepoIdConverter sub_converter(sub->get_id());
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Publication::remove_associated_subscription: ")
               ACE_TEXT("publication %C failed to remove subscription %C at %x.\n"),
               std::string(pub_converter).c_str(),
               std::string(sub_converter).c_str(),
               sub));
  } // if (0 == status)

  if (marked_dead) {
    return -1;

  } else {
    return status;
  }
}

int DCPS_IR_Publication::remove_associations(CORBA::Boolean notify_lost)
{
  int status = 0;
  DCPS_IR_Subscription* sub = 0;
  size_t numAssociations = associations_.size();
  CORBA::Boolean dontSend = 0;
  CORBA::Boolean send = 1;

  if (0 < numAssociations) {
    DCPS_IR_Subscription_Set::ITERATOR iter = associations_.begin();
    DCPS_IR_Subscription_Set::ITERATOR end = associations_.end();

    while (iter != end) {
      sub = *iter;
      ++iter;
      sub->remove_associated_publication(this, send, notify_lost);
      remove_associated_subscription(sub, dontSend, notify_lost);
    }
  }
  this->defunct_.reset();

  return status;
}

void DCPS_IR_Publication::disassociate_participant(OpenDDS::DCPS::RepoId id,
                                                   bool reassociate)
{
  DCPS_IR_Subscription* sub = 0;
  size_t numAssociations = associations_.size();
  CORBA::Boolean send = 1;
  CORBA::Boolean dontSend = 0;
  long count = 0;

  if (0 < numAssociations) {
    OpenDDS::DCPS::ReaderIdSeq idSeq(static_cast<CORBA::ULong>(numAssociations));
    idSeq.length(static_cast<CORBA::ULong>(numAssociations));

    DCPS_IR_Subscription_Set::ITERATOR iter = associations_.begin();
    DCPS_IR_Subscription_Set::ITERATOR end = associations_.end();

    while (iter != end) {
      sub = *iter;
      ++iter;

      if (OpenDDS::DCPS::DCPS_debug_level > 0) {
        OpenDDS::DCPS::RepoIdConverter pub_converter(id_);
        OpenDDS::DCPS::RepoIdConverter sub_converter(sub->get_id());
        OpenDDS::DCPS::RepoIdConverter pub_part_converter(id);
        OpenDDS::DCPS::RepoIdConverter sub_part_converter(sub->get_participant_id());
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) DCPS_IR_Publication::disassociate_participant: ")
                   ACE_TEXT("publication %C testing if subscription %C particpant %C == %C.\n"),
                   std::string(pub_converter).c_str(),
                   std::string(sub_converter).c_str(),
                   std::string(sub_part_converter).c_str(),
                   std::string(pub_part_converter).c_str()));
      }

      if (id == sub->get_participant_id()) {
        CORBA::Boolean dont_notify_lost = 0;
        sub->remove_associated_publication(this, send, dont_notify_lost);
        remove_associated_subscription(sub, dontSend, dont_notify_lost);

        idSeq[count] = sub->get_id();
        ++count;

        if (reassociate && this->defunct_.insert(sub) != 0) {
          OpenDDS::DCPS::RepoIdConverter pub_converter(id_);
          OpenDDS::DCPS::RepoIdConverter sub_converter(sub->get_id());
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Publication::disassociate_participant: ")
                     ACE_TEXT("publication %C failed to reassociate subscription %C at %x.\n"),
                     std::string(pub_converter).c_str(),
                     std::string(sub_converter).c_str(),
                     sub));
        }
      }
    }

    if (0 < count) {
      idSeq.length(count);

      if (participant_->is_alive() && this->participant_->isOwner()) {
        try {
          CORBA::Boolean dont_notify_lost = 0;
          writer_->remove_associations(idSeq, dont_notify_lost);

        } catch (const CORBA::Exception& ex) {
          if (OpenDDS::DCPS::DCPS_debug_level > 0) {
            ex._tao_print_exception(
              "(%P|%t) ERROR: Exception caught in DCPS_IR_Publication::disassociate_participant:");
          }

          participant_->mark_dead();
        }
      }
    }
  }
}

void DCPS_IR_Publication::disassociate_topic(OpenDDS::DCPS::RepoId id)
{
  DCPS_IR_Subscription* sub = 0;
  size_t numAssociations = associations_.size();
  CORBA::Boolean send = 1;
  CORBA::Boolean dontSend = 0;
  long count = 0;

  if (0 < numAssociations) {
    OpenDDS::DCPS::ReaderIdSeq idSeq(static_cast<CORBA::ULong>(numAssociations));
    idSeq.length(static_cast<CORBA::ULong>(numAssociations));

    DCPS_IR_Subscription_Set::ITERATOR iter = associations_.begin();
    DCPS_IR_Subscription_Set::ITERATOR end = associations_.end();

    while (iter != end) {
      sub = *iter;
      ++iter;

      if (OpenDDS::DCPS::DCPS_debug_level > 0) {
        OpenDDS::DCPS::RepoIdConverter pub_converter(id_);
        OpenDDS::DCPS::RepoIdConverter sub_converter(sub->get_id());
        OpenDDS::DCPS::RepoIdConverter pub_topic_converter(id);
        OpenDDS::DCPS::RepoIdConverter sub_topic_converter(sub->get_topic_id());
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) DCPS_IR_Publication::disassociate_topic: ")
                   ACE_TEXT("publication %C testing if subscription %C topic %C == %C.\n"),
                   std::string(pub_converter).c_str(),
                   std::string(sub_converter).c_str(),
                   std::string(sub_topic_converter).c_str(),
                   std::string(pub_topic_converter).c_str()));
      }

      if (id == sub->get_topic_id()) {
        CORBA::Boolean dont_notify_lost = 0;
        sub->remove_associated_publication(this, send, dont_notify_lost);
        remove_associated_subscription(sub, dontSend, dont_notify_lost);

        idSeq[count] = sub->get_id();
        ++count;
      }
    }

    if (0 < count) {
      idSeq.length(count);

      if (participant_->is_alive() && this->participant_->isOwner()) {
        try {
          CORBA::Boolean dont_notify_lost = 0;
          writer_->remove_associations(idSeq, dont_notify_lost);

        } catch (const CORBA::Exception& ex) {
          if (OpenDDS::DCPS::DCPS_debug_level > 0) {
            ex._tao_print_exception(
              "(%P|%t) ERROR: Exception caught in DCPS_IR_Publication::remove_associations:");
          }

          participant_->mark_dead();
        }
      }
    }
  }
}

void DCPS_IR_Publication::disassociate_subscription(OpenDDS::DCPS::RepoId id,
                                                    bool reassociate)
{
  DCPS_IR_Subscription* sub = 0;
  size_t numAssociations = associations_.size();
  CORBA::Boolean send = 1;
  CORBA::Boolean dontSend = 0;
  long count = 0;

  if (0 < numAssociations) {
    OpenDDS::DCPS::ReaderIdSeq idSeq(static_cast<CORBA::ULong>(numAssociations));
    idSeq.length(static_cast<CORBA::ULong>(numAssociations));

    DCPS_IR_Subscription_Set::ITERATOR iter = associations_.begin();
    DCPS_IR_Subscription_Set::ITERATOR end = associations_.end();

    while (iter != end) {
      sub = *iter;
      ++iter;

      if (OpenDDS::DCPS::DCPS_debug_level > 0) {
        OpenDDS::DCPS::RepoIdConverter pub_converter(id_);
        OpenDDS::DCPS::RepoIdConverter sub_converter(sub->get_id());
        OpenDDS::DCPS::RepoIdConverter pub_sub_converter(id);
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) DCPS_IR_Publication::disassociate_subscription: ")
                   ACE_TEXT("publication %C testing if subscription %C == %C.\n"),
                   std::string(pub_converter).c_str(),
                   std::string(sub_converter).c_str(),
                   std::string(pub_sub_converter).c_str()));
      }

      if (id == sub->get_id()) {
        CORBA::Boolean dont_notify_lost = 0;
        sub->remove_associated_publication(this, send, dont_notify_lost);
        remove_associated_subscription(sub, dontSend, dont_notify_lost);

        idSeq[count] = sub->get_id();
        ++count;

        if (reassociate && this->defunct_.insert(sub) != 0) {
          OpenDDS::DCPS::RepoIdConverter pub_converter(id_);
          OpenDDS::DCPS::RepoIdConverter sub_converter(sub->get_id());
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Publication::disassociate_subscription: ")
                     ACE_TEXT("publication %C failed to reassociate subscription %C at %x.\n"),
                     std::string(pub_converter).c_str(),
                     std::string(sub_converter).c_str(),
                     sub));
        }
      }
    }

    if (0 < count) {
      idSeq.length(count);

      if (participant_->is_alive() && this->participant_->isOwner()) {
        try {
          CORBA::Boolean dont_notify_lost = 0;
          writer_->remove_associations(idSeq, dont_notify_lost);

        } catch (const CORBA::Exception& ex) {
          if (OpenDDS::DCPS::DCPS_debug_level > 0) {
            ex._tao_print_exception(
              "(%P|%t) ERROR: Exception caught in DCPS_IR_Publication::remove_associations:");
          }

          participant_->mark_dead();
        }
      }
    }
  }
}

void DCPS_IR_Publication::update_incompatible_qos()
{
  if (participant_->is_alive() && this->participant_->isOwner()) {
    try {
      writer_->update_incompatible_qos(incompatibleQosStatus_);
      incompatibleQosStatus_.count_since_last_send = 0;
    } catch (const CORBA::Exception& ex) {
      if (OpenDDS::DCPS::DCPS_debug_level > 0) {
        ex._tao_print_exception(
          "(%P|%t) ERROR: Exception caught in DCPS_IR_Publication::update_incompatible_qos:");
      }

      participant_->mark_dead();
    }
  }
}

CORBA::Boolean DCPS_IR_Publication::is_subscription_ignored(OpenDDS::DCPS::RepoId partId,
                                                            OpenDDS::DCPS::RepoId topicId,
                                                            OpenDDS::DCPS::RepoId subId)
{
  CORBA::Boolean ignored;
  ignored = (participant_->is_participant_ignored(partId) ||
             participant_->is_topic_ignored(topicId) ||
             participant_->is_subscription_ignored(subId));

  return ignored;
}

DDS::DataWriterQos* DCPS_IR_Publication::get_datawriter_qos()
{
  return &qos_;
}

using OpenDDS::DCPS::operator==;

void
DCPS_IR_Publication::set_qos(const DDS::DataWriterQos& qos)
{
  if (false == (qos == this->qos_)) {
    // Check if we should check while we have both values.
    bool check = OpenDDS::DCPS::should_check_association_upon_change(qos, this->qos_);

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

    this->participant_->get_domain_reference()->publish_publication_bit(this);
  }
}

void
DCPS_IR_Publication::set_qos(const DDS::PublisherQos& qos)
{
  if (false == (qos == this->publisherQos_)) {
    // Check if we should check while we have both values.
    bool check = OpenDDS::DCPS::should_check_association_upon_change(qos, this->publisherQos_);

    // Store the new, compatible, value.
    this->publisherQos_ = qos;

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

    this->participant_->get_domain_reference()->publish_publication_bit(this);
  }
}

bool DCPS_IR_Publication::set_qos(const DDS::DataWriterQos & qos,
                                  const DDS::PublisherQos & publisherQos,
                                  Update::SpecificQos& specificQos)
{
  bool need_evaluate = false;
  bool u_dw_qos = !(qos_ == qos);

  if (u_dw_qos) {
    if (OpenDDS::DCPS::should_check_association_upon_change(qos_, qos)) {
      need_evaluate = true;
    }

    qos_ = qos;
  }

  bool u_pub_qos = !(publisherQos_ == publisherQos);

  if (u_pub_qos) {
    if (OpenDDS::DCPS::should_check_association_upon_change(publisherQos_, publisherQos)) {
      need_evaluate = true;
    }

    publisherQos_ = publisherQos;
  }

  if (need_evaluate) {
    // Check if any existing association need be removed first.
    this->reevaluate_existing_associations();

    DCPS_IR_Topic_Description* description = this->topic_->get_topic_description();
    description->reevaluate_associations(this);
  }

  participant_->get_domain_reference()->publish_publication_bit(this);
  specificQos = u_dw_qos?  Update::DataWriterQos:
                u_pub_qos? Update::PublisherQos:
                Update::NoQos;

  return true;
}

DDS::PublisherQos* DCPS_IR_Publication::get_publisher_qos()
{
  return &publisherQos_;
}

OpenDDS::DCPS::TransportLocatorSeq DCPS_IR_Publication::get_transportLocatorSeq() const
{
  return info_;
}

OpenDDS::DCPS::IncompatibleQosStatus* DCPS_IR_Publication::get_incompatibleQosStatus()
{
  return &incompatibleQosStatus_;
}

OpenDDS::DCPS::RepoId DCPS_IR_Publication::get_id()
{
  return id_;
}

OpenDDS::DCPS::RepoId DCPS_IR_Publication::get_topic_id()
{
  return topic_->get_id();
}

OpenDDS::DCPS::RepoId DCPS_IR_Publication::get_participant_id()
{
  return participant_->get_id();
}

DCPS_IR_Topic* DCPS_IR_Publication::get_topic()
{
  return topic_;
}

DCPS_IR_Topic_Description* DCPS_IR_Publication::get_topic_description()
{
  return topic_->get_topic_description();
}

DDS::InstanceHandle_t DCPS_IR_Publication::get_handle()
{
  return handle_;
}

void DCPS_IR_Publication::set_handle(DDS::InstanceHandle_t handle)
{
  handle_ = handle;
}

CORBA::Boolean DCPS_IR_Publication::is_bit()
{
  return isBIT_;
}

void DCPS_IR_Publication::set_bit_status(CORBA::Boolean isBIT)
{
  isBIT_ = isBIT;
}

OpenDDS::DCPS::DataWriterRemote_ptr
DCPS_IR_Publication::writer()
{
  return OpenDDS::DCPS::DataWriterRemote::_duplicate(this->writer_.in());
}

void
DCPS_IR_Publication::reevaluate_defunct_associations()
{
  DCPS_IR_Subscription_Set::iterator it(this->defunct_.begin());
  while (it != this->defunct_.end()) {
    DCPS_IR_Subscription* subscription = *it;
    ++it;

    if (reevaluate_association(subscription)) {
      this->defunct_.remove(subscription); // no longer defunct

    } else {
      OpenDDS::DCPS::RepoIdConverter pub_converter(id_);
      OpenDDS::DCPS::RepoIdConverter sub_converter(subscription->get_id());
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Publication::reevaluate_defunct_associations: ")
                 ACE_TEXT("publication %C failed to reassociate subscription %C at %x.\n"),
                 std::string(pub_converter).c_str(),
                 std::string(sub_converter).c_str(),
                 subscription));
    }
  }
}

void DCPS_IR_Publication::reevaluate_existing_associations()
{
  DCPS_IR_Subscription* sub = 0;
  DCPS_IR_Subscription_Set::ITERATOR iter = associations_.begin();
  DCPS_IR_Subscription_Set::ITERATOR end = associations_.end();

  while (iter != end) {
    sub = *iter;
    ++iter;

    this->reevaluate_association(sub);
  }
}

bool
DCPS_IR_Publication::reevaluate_association(DCPS_IR_Subscription* subscription)
{
  int status = this->associations_.find(subscription);

  if (status == 0) {
    // verify if they are still compatiable after change


    if (!OpenDDS::DCPS::compatibleQOS(this->get_incompatibleQosStatus(),
                                      subscription->get_incompatibleQosStatus(),
                                      this->get_transportLocatorSeq(),
                                      subscription->get_transportLocatorSeq(),
                                      this->get_datawriter_qos(),
                                      subscription->get_datareader_qos(),
                                      this->get_publisher_qos(),
                                      subscription->get_subscriber_qos())) {
      bool sendNotify = true; // inform datawriter
      bool notify_lost = true; // invoke listerner callback

      this->remove_associated_subscription(subscription, sendNotify, notify_lost, true);
    }

  } else {
    DCPS_IR_Topic_Description* description = this->topic_->get_topic_description();
    return description->try_associate(this, subscription);
  }

  return false;
}

void
DCPS_IR_Publication::update_expr_params(OpenDDS::DCPS::RepoId readerId,
                                        const DDS::StringSeq& params)
{
  try {
    writer_->update_subscription_params(readerId, params);
  } catch (const CORBA::SystemException& ex) {
    if (OpenDDS::DCPS::DCPS_debug_level) {
      ex._tao_print_exception("(%P|%t) ERROR: Exception caught in "
        "DCPS_IR_Publication::update_expr_params:");
    }
    participant_->mark_dead();
  }
}

std::string
DCPS_IR_Publication::dump_to_string(const std::string& prefix, int depth) const
{
  std::string str;
#if !defined (OPENDDS_INFOREPO_REDUCED_FOOTPRINT)
  OpenDDS::DCPS::RepoIdConverter local_converter(id_);

  for (int i=0; i < depth; i++)
    str += prefix;
  std::string indent = str + prefix;
  str += "DCPS_IR_Publication[";
  str += std::string(local_converter);
  str += "]";
  if (isBIT_)
    str += " (BIT)";
  str += "\n";

  str += indent + "Associations [ ";
  for (DCPS_IR_Subscription_Set::const_iterator assoc = associations_.begin();
       assoc != associations_.end();
       assoc++)
  {
    OpenDDS::DCPS::RepoIdConverter assoc_converter((*assoc)->get_id());
    str += std::string(assoc_converter);
    str += " ";
  }
  str += "]\n";

  str += indent + "Defunct Associations [ ";
  for (DCPS_IR_Subscription_Set::const_iterator def = defunct_.begin();
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
