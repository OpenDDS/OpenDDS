// ============================================================================
/**
 *  @file   DCPS_IR_Topic_Description.h
 *
 *  $Id$
 *
 *
 */
// ============================================================================
#ifndef DCPS_IR_TOPIC_DESCRIPTION_H
#define DCPS_IR_TOPIC_DESCRIPTION_H

#include  "inforepo_export.h"
#include /**/ "ace/Unbounded_Set.h"
#include /**/ "ace/SString.h"
#include /**/ "tao/corbafwd.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


// forward declarations
class DCPS_IR_Publication;
class DCPS_IR_Domain;

class DCPS_IR_Subscription;
typedef ACE_Unbounded_Set<DCPS_IR_Subscription*> DCPS_IR_Subscription_Set;

class DCPS_IR_Topic;
typedef ACE_Unbounded_Set<DCPS_IR_Topic*> DCPS_IR_Topic_Set;


/**
 * @class DCPS_IR_Topic_Description
 *
 * @brief Representative of a Topic Description
 *
 */
class OpenDDS_InfoRepoLib_Export DCPS_IR_Topic_Description
{
public:
  DCPS_IR_Topic_Description (
    DCPS_IR_Domain* domain,
    const char* name,
    const char* dataTypeName);

  ~DCPS_IR_Topic_Description (void);

  /// Adds the subscription to the list of subscriptions
  /// Tries to associate with existing publications if successfully added
  /// 'associate' switch toggles association attempt.
  /// Returns 0 if added, 1 if already exists, -1 other failure
  int add_subscription_reference (DCPS_IR_Subscription* subscription
				  , bool associate = true);

  /// Removes the subscription from the list of subscriptions
  /// Returns 0 if successful
  int remove_subscription_reference (DCPS_IR_Subscription* subscription);

  /// Add a topic
  /// Takes ownership of memory pointed to by topic
  /// Returns 0 if added, 1 if already exists, -1 other failure
  int add_topic (DCPS_IR_Topic* topic);

  /// Removes the topic from the list of topics
  /// Gives ownership of the memory pointed to by topic
  ///  to the caller
  /// Returns 0 if successful
  int remove_topic (DCPS_IR_Topic* topic);

  /// Gets the first topic in the topic list
  DCPS_IR_Topic* get_first_topic ();

  /// Tries to associate the publication will each of
  ///  the subscriptions in the subscription list
  void try_associate_publication (DCPS_IR_Publication* publication);

  /// Tries to associate the subscription will each of
  ///  the publications in each topic in the topic list
  void try_associate_subscription (DCPS_IR_Subscription* subscription);

  /// Checks to see if the publication and subscription can
  ///  be associated.
  void try_associate (DCPS_IR_Publication* publication,
                      DCPS_IR_Subscription* subscription);

  /// Associate the publication and subscription
  void associate (DCPS_IR_Publication* publication,
                  DCPS_IR_Subscription* subscription);

  /// Re-evaluate the association between the provided publication and
  /// the subscriptions it maintains.
  void reevaluate_associations (DCPS_IR_Publication* publication);

  /// Re-evaluate the association between the provided subscription and
  /// the publications in all its maintained topics.  
  void reevaluate_associations (DCPS_IR_Subscription* subscription);

  const char* get_name () const ;
  const char* get_dataTypeName () const;

  /// Returns the number of topics
  CORBA::ULong get_number_topics () const;

private:
  ACE_SString name_;
  ACE_SString dataTypeName_;
  DCPS_IR_Domain* domain_;

  DCPS_IR_Subscription_Set subscriptionRefs_;
  DCPS_IR_Topic_Set topics_;
};

#endif /* DCPS_IR_TOPIC_DESCRIPTION_H */
