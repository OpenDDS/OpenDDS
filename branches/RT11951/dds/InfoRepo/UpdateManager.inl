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

  // serialize the Topic QOS
  TAO_OutputCDR outCdr;
  outCdr << actor.pubsubQos;
  size_t len = outCdr.total_length();
  char *buf = new char[len];
  ArrDelAdapter<char> guard (buf);
  if (buf == 0) {
    ACE_ERROR ((LM_ERROR, "UpdateManager::add actor> Allocation failed.\n"));
    return;
  }

  ACE_Message_Block dst;
  ACE_CDR::consolidate (&dst, outCdr.begin ());
  ACE_OS::memcpy (buf, dst.base(), len);

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

  BinSeq pubsub_qos_bin (len, buf);
  QosSeq pubsub_qos (pubsubQosType, pubsub_qos_bin);



  outCdr.reset ();
  outCdr << actor.drdwQos;
  len = outCdr.total_length();
  char *buf2 = new char[len];

  ArrDelAdapter<char> guard2 (buf2);
  if (buf2 == 0) {
    ACE_ERROR ((LM_ERROR, "UpdateManager::add actor> Allocation failed.\n"));
    return;
  }

  ACE_CDR::consolidate (&dst, outCdr.begin ());
  ACE_OS::memcpy (buf2, dst.base(), len);

  BinSeq dwdr_qos_bin (len, buf2);
  QosSeq dwdr_qos (dwdrQosType, dwdr_qos_bin);


  outCdr.reset ();
  outCdr << actor.transportInterfaceInfo;
  len = outCdr.total_length();
  char *buf3 = new char[len];
  ArrDelAdapter<char> guard3 (buf3);
  if (buf3 == 0) {
    ACE_ERROR ((LM_ERROR, "UpdateManager::add actor> Allocation failed.\n"));
    return;
  }

  ACE_CDR::consolidate (&dst, outCdr.begin ());
  ACE_OS::memcpy (buf3, dst.base(), len);

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
