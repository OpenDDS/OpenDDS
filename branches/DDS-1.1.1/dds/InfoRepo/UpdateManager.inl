// -*- C++ -*-
#include "ArrDelAdapter.h"

#include <memory>

template <typename ActorType, typename UA> void
UpdateManager::add(const ActorType& actorType, const UA& actor)
{
  if (actorType != DataWriter && actorType != DataReader)
  {
    ACE_ERROR ((LM_ERROR, "(%P|%t)ERROR: UpdateManager::add invalid actor type %d\n", actorType));
    return;
  }

  if (updaters_.empty()) {
    return;
  }

  // setup qos type
  SpecificQos pubsubQosType;
  SpecificQos dwdrQosType;
  if (actorType == DataWriter)
  {
    pubsubQosType = PublisherQos;
    dwdrQosType = DataWriterQos;
  }
  else
  {
    pubsubQosType = SubscriberQos;
    dwdrQosType = DataReaderQos;
  }

  // serialize the Topic QOS
  TAO_OutputCDR outCdr;
  outCdr << actor.pubsubQos;
  ACE_Message_Block dst;
  ACE_CDR::consolidate (&dst, outCdr.begin ());

  size_t len = dst.length();
  char *buf = new char[len];
  if (buf == 0) {
    ACE_ERROR ((LM_ERROR, "UpdateManager::add actor> Allocation failed.\n"));
    return;
  }

  ArrDelAdapter<char> guard (buf);

  ACE_OS::memcpy (buf, dst.base(), len);


  BinSeq pubsub_qos_bin (len, buf);
  QosSeq pubsub_qos (pubsubQosType, pubsub_qos_bin);


  outCdr.reset ();
  outCdr << actor.drdwQos;
  ACE_Message_Block dst2;
  ACE_CDR::consolidate (&dst2, outCdr.begin ());

  len = dst2.length();
  char *buf2 = new char[len];

  if (buf2 == 0) {
    ACE_ERROR ((LM_ERROR, "UpdateManager::add actor> Allocation failed.\n"));
    return;
  }

  ArrDelAdapter<char> guard2 (buf2);

  ACE_OS::memcpy (buf2, dst2.base(), len);

  BinSeq dwdr_qos_bin (len, buf2);
  QosSeq dwdr_qos (dwdrQosType, dwdr_qos_bin);


  outCdr.reset ();
  outCdr << actor.transportInterfaceInfo;
  ACE_Message_Block dst3;
  ACE_CDR::consolidate (&dst3, outCdr.begin ());

  len = dst3.length();
  char *buf3 = new char[len];
  if (buf3 == 0) {
    ACE_ERROR ((LM_ERROR, "UpdateManager::add actor> Allocation failed.\n"));
    return;
  }

  ArrDelAdapter<char> guard3 (buf3);

  ACE_OS::memcpy (buf3, dst3.base(), len);
  BinSeq tr_bin (len, buf3);


  DActor actor_data (actor.domainId, actor.actorId, actor.topicId
                     , actor.participantId
                     , actorType, actor.callback.c_str(), pubsub_qos
                     , dwdr_qos, tr_bin);

  // Invoke add on each of the iterators.
  for (Updaters::iterator iter = updaters_.begin();
    iter != updaters_.end();
    iter++) {
      this->add (*iter, actor_data);
    }
}
