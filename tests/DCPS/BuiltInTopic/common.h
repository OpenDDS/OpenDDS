// -*- C++ -*-
//
#ifndef COMMON_H
#define COMMON_H

#include "dds/DCPS/DomainParticipantImpl.h"
#include "dds/DCPS/TopicImpl.h"
#include "dds/DCPS/DataWriterImpl.h"
#include "dds/DCPS/DataReaderImpl.h"
#include "dds/DCPS/PublisherImpl.h"
#include "dds/DCPS/SubscriberImpl.h"
#include "dds/DdsDcpsDomainC.h"

using namespace ::DDS;
using namespace ::OpenDDS::DCPS;

extern const long  TEST_DOMAIN ;
extern const char* TEST_TOPIC;
extern const char* TEST_TOPIC_TYPE;
extern const ACE_TCHAR* reader_address_str;
extern const ACE_TCHAR* writer_address_str;
extern int default_key;
extern int num_writes;
extern Subscriber_var bit_subscriber;
extern DomainParticipant_var participant;
extern DomainParticipantImpl* participant_servant;
extern Topic_var topic;
extern TopicImpl* topic_servant;
extern Publisher_var publisher;
extern PublisherImpl* publisher_servant;
extern DataWriter_var datawriter;
extern DataWriterImpl* datawriter_servant;
extern Subscriber_var subscriber;
extern SubscriberImpl* subscriber_servant;
extern DataReader_var datareader;
extern DataReaderImpl* datareader_servant;
extern DomainParticipantFactory_var participant_factory;


enum Ignore_Kind
{
  DONT_IGNORE,
  IGNORE_PARTICIPANT,
  IGNORE_TOPIC,
  IGNORE_PUBLICATION,
  IGNORE_SUBSCRIPTION
};

extern int ignore_kind;

int ignore ();
int write ();
int read (int expect_success);

#endif /* COMMON_H */

