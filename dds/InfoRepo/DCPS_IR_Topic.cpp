/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DcpsInfo_pch.h"

#include /**/ "DCPS_IR_Topic.h"
#include /**/ "DCPS_IR_Domain.h"

#include /**/ "DCPS_IR_Subscription.h"
#include /**/ "DCPS_IR_Publication.h"
#include /**/ "DCPS_IR_Participant.h"
#include /**/ "DCPS_IR_Topic_Description.h"

#include /**/ "dds/DCPS/DCPS_Utils.h"
#include /**/ "dds/DCPS/RepoIdConverter.h"
#include /**/ "dds/DCPS/Qos_Helper.h"
#include /**/ "tao/debug.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

DCPS_IR_Topic::DCPS_IR_Topic(const OpenDDS::DCPS::RepoId& id,
                             const DDS::TopicQos& qos,
                             DCPS_IR_Domain* domain,
                             DCPS_IR_Participant* creator,
                             DCPS_IR_Topic_Description* description)
  : id_(id),
    qos_(qos),
    domain_(domain),
    participant_(creator),
    description_(description),
    handle_(0),
    isBIT_(0),
    removed_(false)
{
}

DCPS_IR_Topic::~DCPS_IR_Topic()
{
  // check for remaining publication references
  if (0 != publicationRefs_.size()) {
    DCPS_IR_Publication* pub = 0;
    DCPS_IR_Publication_Set::ITERATOR iter = publicationRefs_.begin();
    DCPS_IR_Publication_Set::ITERATOR end = publicationRefs_.end();

    OpenDDS::DCPS::RepoIdConverter topic_converter(id_);
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Topic::~DCPS_IR_Topic: ")
               ACE_TEXT("id %C has retained publications.\n"),
               std::string(topic_converter).c_str()));

    while (iter != end) {
      pub = *iter;
      ++iter;

      OpenDDS::DCPS::RepoIdConverter pub_converter(pub->get_id());
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Topic::~DCPS_IR_Topic: ")
                 ACE_TEXT("topic %C retains publication id %C.\n"),
                 std::string(topic_converter).c_str(),
                 std::string(pub_converter).c_str()));
    }
  }

  if (0 != subscriptionRefs_.size()) {
    DCPS_IR_Subscription* sub = 0;
    DCPS_IR_Subscription_Set::ITERATOR iter = subscriptionRefs_.begin();
    DCPS_IR_Subscription_Set::ITERATOR end = subscriptionRefs_.end();

    OpenDDS::DCPS::RepoIdConverter topic_converter(id_);
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Topic::~DCPS_IR_Topic: ")
               ACE_TEXT("id %C has retained subscriptions.\n"),
               std::string(topic_converter).c_str()));

    while (iter != end) {
      sub = *iter;
      ++iter;

      OpenDDS::DCPS::RepoIdConverter sub_converter(sub->get_id());
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Topic::~DCPS_IR_Topic: ")
                 ACE_TEXT("topic %C retains subscription id %C.\n"),
                 std::string(topic_converter).c_str(),
                 std::string(sub_converter).c_str()));
    }
  }
}

void DCPS_IR_Topic::release(bool removing)
{
  if (removing) {
    this->removed_ = true;

    if (publicationRefs_.size() == 0 && subscriptionRefs_.size() == 0) {
      this->domain_->remove_topic_id_mapping(this->id_);
      delete this;
    }

  } else if (this->removed_) {
    if (publicationRefs_.size() == 0 && subscriptionRefs_.size() == 0) {
      this->domain_->remove_topic_id_mapping(this->id_);
      delete this;
    }
  }
}

int DCPS_IR_Topic::add_publication_reference(DCPS_IR_Publication* publication
                                             , bool associate)
{
  int status = publicationRefs_.insert(publication);

  switch (status) {
  case 0:

    // Publish the BIT information
    domain_->publish_publication_bit(publication);

    if (associate) {
      description_->try_associate_publication(publication);
      // Do not check incompatible qos here.  The check is done
      // in the DCPS_IR_Topic_Description::try_associate_publication method
    }

    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      OpenDDS::DCPS::RepoIdConverter topic_converter(id_);
      OpenDDS::DCPS::RepoIdConverter pub_converter(publication->get_id());
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DCPS_IR_Topic::add_publication_reference: ")
                 ACE_TEXT("topic %C added publication %C at %x\n"),
                 std::string(topic_converter).c_str(),
                 std::string(pub_converter).c_str(),
                 publication));
    }

    break;

  case 1:

    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      OpenDDS::DCPS::RepoIdConverter topic_converter(id_);
      OpenDDS::DCPS::RepoIdConverter pub_converter(publication->get_id());
      ACE_DEBUG((LM_WARNING,
                 ACE_TEXT("(%P|%t) WARNING: DCPS_IR_Topic::add_publication_reference: ")
                 ACE_TEXT("topic %C attempt to re-add publication %C.\n"),
                 std::string(topic_converter).c_str(),
                 std::string(pub_converter).c_str()));
    }

    break;

  case -1: {
    OpenDDS::DCPS::RepoIdConverter topic_converter(id_);
    OpenDDS::DCPS::RepoIdConverter pub_converter(publication->get_id());
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Topic::add_publication_reference: ")
               ACE_TEXT("topic %C failed to add publication %C\n"),
               std::string(topic_converter).c_str(),
               std::string(pub_converter).c_str()));
  }
  };

  return status;
}

int DCPS_IR_Topic::remove_publication_reference(DCPS_IR_Publication* publication)
{
  int status = publicationRefs_.remove(publication);

  if (0 == status) {
    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      OpenDDS::DCPS::RepoIdConverter topic_converter(id_);
      OpenDDS::DCPS::RepoIdConverter pub_converter(publication->get_id());
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DCPS_IR_Topic::remove_publication_reference: ")
                 ACE_TEXT("topic %C removed publication %C.\n"),
                 std::string(topic_converter).c_str(),
                 std::string(pub_converter).c_str()));
    }

  } else {
    OpenDDS::DCPS::RepoIdConverter topic_converter(id_);
    OpenDDS::DCPS::RepoIdConverter pub_converter(publication->get_id());
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Topic::remove_publication_reference: ")
               ACE_TEXT("topic %C failed to remove publication %C.\n"),
               std::string(topic_converter).c_str(),
               std::string(pub_converter).c_str()));
  }

  return status;
}

int DCPS_IR_Topic::add_subscription_reference(DCPS_IR_Subscription* subscription
                                              , bool associate)
{
  int status = subscriptionRefs_.insert(subscription);

  switch (status) {
  case 0:

    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      OpenDDS::DCPS::RepoIdConverter topic_converter(id_);
      OpenDDS::DCPS::RepoIdConverter sub_converter(subscription->get_id());
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DCPS_IR_Topic::add_subscription_reference: ")
                 ACE_TEXT("topic %C added subscription %C at %x.\n"),
                 std::string(topic_converter).c_str(),
                 std::string(sub_converter).c_str(),
                 subscription));
    }

    status = this->description_->add_subscription_reference(subscription, associate);
    break;

  case 1: {
    OpenDDS::DCPS::RepoIdConverter topic_converter(id_);
    OpenDDS::DCPS::RepoIdConverter sub_converter(subscription->get_id());
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Topic::add_subscription_reference: ")
               ACE_TEXT("topic %C attempt to re-add subscription %C.\n"),
               std::string(topic_converter).c_str(),
               std::string(sub_converter).c_str()));
  }
  break;

  case -1: {
    OpenDDS::DCPS::RepoIdConverter topic_converter(id_);
    OpenDDS::DCPS::RepoIdConverter sub_converter(subscription->get_id());
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Topic::add_subscription_reference: ")
               ACE_TEXT("topic %C failed to add subscription %C.\n"),
               std::string(topic_converter).c_str(),
               std::string(sub_converter).c_str()));
  }
  };

  return status;
}

int DCPS_IR_Topic::remove_subscription_reference(DCPS_IR_Subscription* subscription)
{
  int status = subscriptionRefs_.remove(subscription);

  if (0 == status) {
    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      OpenDDS::DCPS::RepoIdConverter topic_converter(id_);
      OpenDDS::DCPS::RepoIdConverter sub_converter(subscription->get_id());
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DCPS_IR_Topic::remove_subscription_reference: ")
                 ACE_TEXT("topic %C removed subscription %C.\n"),
                 std::string(topic_converter).c_str(),
                 std::string(sub_converter).c_str()));
    }

    this->description_->remove_subscription_reference(subscription);

  } else {
    OpenDDS::DCPS::RepoIdConverter topic_converter(id_);
    OpenDDS::DCPS::RepoIdConverter sub_converter(subscription->get_id());
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Topic::remove_subscription_reference: ")
               ACE_TEXT("topic %C failed to remove subscription %C.\n"),
               std::string(topic_converter).c_str(),
               std::string(sub_converter).c_str()));
  } // if (0 == status)

  return status;
}

OpenDDS::DCPS::RepoId DCPS_IR_Topic::get_id() const
{
  return id_;
}

OpenDDS::DCPS::RepoId DCPS_IR_Topic::get_participant_id() const
{
  return participant_->get_id();
}

DDS::TopicQos * DCPS_IR_Topic::get_topic_qos()
{
  return &qos_;
}

bool DCPS_IR_Topic::set_topic_qos(const DDS::TopicQos& qos)
{
  // Do not need re-evaluate compatibility and associations when
  // TopicQos changes since only datareader and datawriter QoS
  // are evaludated during normal associations establishment.
  using OpenDDS::DCPS::operator==;
  bool pub_to_rd_wr = !(qos.topic_data == qos_.topic_data);

  qos_ = qos;
  domain_->publish_topic_bit(this);

  if (!pub_to_rd_wr)
    return true;

  // The only changeable TopicQos used by DataWriter and DataReader
  // is topic_data so we need publish it to DW/DR BIT to make they
  // are consistent.

  // Update qos in datawriter BIT for associated datawriters.

  {
    DCPS_IR_Publication_Set::ITERATOR iter = publicationRefs_.begin();
    DCPS_IR_Publication_Set::ITERATOR end = publicationRefs_.end();

    while (iter != end) {
      domain_->publish_publication_bit(*iter);
      ++iter;
    }
  }

  // Update qos in datareader BIT for associated datareader.

  {
    DCPS_IR_Subscription_Set::ITERATOR iter = subscriptionRefs_.begin();
    DCPS_IR_Subscription_Set::ITERATOR end = subscriptionRefs_.end();

    while (iter != end) {
      domain_->publish_subscription_bit(*iter);
      ++iter;
    }
  }

  return true;
}

void DCPS_IR_Topic::try_associate(DCPS_IR_Subscription* subscription)
{
  // check if we should ignore this subscription
  if (participant_->is_subscription_ignored(subscription->get_id()) ||
      participant_->is_participant_ignored(subscription->get_participant_id()) ||
      participant_->is_topic_ignored(subscription->get_topic_id())) {
    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      OpenDDS::DCPS::RepoIdConverter topic_converter(id_);
      OpenDDS::DCPS::RepoIdConverter sub_converter(subscription->get_id());
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DCPS_IR_Topic::try_associate: ")
                 ACE_TEXT("topic %C ignoring subscription %C.\n"),
                 std::string(topic_converter).c_str(),
                 std::string(sub_converter).c_str()));
    }

  } else {
    // check all publications for compatibility
    DCPS_IR_Publication* pub = 0;
    OpenDDS::DCPS::IncompatibleQosStatus* qosStatus = 0;

    DCPS_IR_Publication_Set::ITERATOR iter = publicationRefs_.begin();
    DCPS_IR_Publication_Set::ITERATOR end = publicationRefs_.end();

    while (iter != end) {
      pub = *iter;
      ++iter;
      description_->try_associate(pub, subscription);
      // Check the publications QOS status
      qosStatus = pub->get_incompatibleQosStatus();

      if (0 < qosStatus->count_since_last_send) {
        pub->update_incompatible_qos();
      }
    } /* while (iter != end) */

    // The subscription QOS is not checked because
    // we don't know if the subscription is finished cycling
    // through topics.
  }
}

DCPS_IR_Topic_Description* DCPS_IR_Topic::get_topic_description()
{
  return description_;
}

DDS::InstanceHandle_t DCPS_IR_Topic::get_handle()
{
  return handle_;
}

void DCPS_IR_Topic::set_handle(DDS::InstanceHandle_t handle)
{
  handle_ = handle;
}

CORBA::Boolean DCPS_IR_Topic::is_bit()
{
  return isBIT_;
}

void DCPS_IR_Topic::set_bit_status(CORBA::Boolean isBIT)
{
  isBIT_ = isBIT;
}

void DCPS_IR_Topic::reevaluate_associations(DCPS_IR_Subscription* subscription)
{
  DCPS_IR_Publication * pub = 0;
  DCPS_IR_Publication_Set::ITERATOR iter = publicationRefs_.begin();
  DCPS_IR_Publication_Set::ITERATOR end = publicationRefs_.end();

  while (iter != end) {
    pub = *iter;
    ++iter;

    subscription->reevaluate_association(pub);
    pub->reevaluate_association(subscription);
  }
}


void DCPS_IR_Topic::reassociate_all_publications()
{
  DCPS_IR_Publication_Set::ITERATOR iter = publicationRefs_.begin();
  DCPS_IR_Publication_Set::ITERATOR end = publicationRefs_.end();

  for ( ; iter != end; ++iter)
  {
    description_->try_associate_publication(*iter);
  }
}

std::string
DCPS_IR_Topic::dump_to_string(const std::string& prefix, int depth) const
{
  std::string str;
#if !defined (OPENDDS_INFOREPO_REDUCED_FOOTPRINT)
  OpenDDS::DCPS::RepoIdConverter local_converter(id_);

  for (int i=0; i < depth; i++)
    str += prefix;
  std::string indent = str + prefix;
  str += "DCPS_IR_Topic[";
  str += std::string(local_converter);
  str += "]";
  if (isBIT_)
    str += " (BIT)";
  str += "\n";

  str += indent + "Publications:\n";
  for (DCPS_IR_Publication_Set::const_iterator pub = publicationRefs_.begin();
       pub != publicationRefs_.end();
       pub++)
  {
    OpenDDS::DCPS::RepoIdConverter pub_converter((*pub)->get_id());
    str += indent + std::string(pub_converter);
    str += "\n";

  }

  str += indent + "Subscriptions:\n";
  for (DCPS_IR_Subscription_Set::const_iterator sub = subscriptionRefs_.begin();
       sub != subscriptionRefs_.end();
       sub++)
  {
    OpenDDS::DCPS::RepoIdConverter sub_converter((*sub)->get_id());
    str += indent + std::string(sub_converter);
    str += "\n";
  }
#endif // !defined (OPENDDS_INFOREPO_REDUCED_FOOTPRINT)
  return str;
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
