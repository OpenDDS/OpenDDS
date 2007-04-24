// -*- C++ -*-
#include "ArrDelAdapter.h"

#include <memory>

template <typename UA> void
UpdateManager::add(const UA& actor)
{
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
  ACE_OS::memcpy (buf, outCdr.buffer(), len);

  BinSeq pub_qos_bin (len, buf);
  QosSeq pub_qos (PublisherQos, pub_qos_bin);



  outCdr.reset ();
  outCdr << actor.drdwQos;
  len = outCdr.total_length();
  char *buf2 = new char[len];
  ArrDelAdapter<char> guard2 (buf2);
  if (buf2 == 0) {
    ACE_ERROR ((LM_ERROR, "UpdateManager::add actor> Allocation failed.\n"));
    return;
  }
  ACE_OS::memcpy (buf2, outCdr.buffer(), len);

  BinSeq dw_qos_bin (len, buf2);
  QosSeq dw_qos (DataWriterQos, dw_qos_bin);



  outCdr.reset ();
  outCdr << actor.transportInterfaceInfo;
  len = outCdr.total_length();
  char *buf3 = new char[len];
  ArrDelAdapter<char> guard3 (buf3);
  if (buf3 == 0) {
    ACE_ERROR ((LM_ERROR, "UpdateManager::add actor> Allocation failed.\n"));
    return;
  }
  ACE_OS::memcpy (buf3, outCdr.buffer(), len);

  BinSeq tr_bin (len, buf3);



  DActor actor_data (actor.domainId, actor.actorId, actor.topicId
                     , actor.participantId
                     , DataWriter, actor.callback.c_str(), pub_qos
                     , dw_qos, tr_bin);

  // Invoke add on each of the iterators.
  for (Updaters::iterator iter = updaters_.begin();
       iter != updaters_.end();
       iter++) {
	this->add (*iter, actor_data);
  }
}
