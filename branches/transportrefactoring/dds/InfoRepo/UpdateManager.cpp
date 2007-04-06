#include "DcpsInfo_pch.h"

#include "UpdateManager.h"

#include "tao/CDR.h"

UpdateManager::UpdateManager (void)
  : info_ (0)
{ }

UpdateManager::~UpdateManager (void)
{
  ACE_DEBUG ((LM_DEBUG, "UpdateManager::~UpdateManager\n"));
}

void
UpdateManager::add (TAO_DDS_DCPSInfo_i* info)
{
  info_ = info;
}

void
UpdateManager::add (Updater* updater)
{
  // push new element to the back.
  updaters_.insert (updater);
}

void
UpdateManager::remove ()
{
  // Clean the refrence to the InfoRepo.
  info_ = 0;
}

void
UpdateManager::remove (const Updater* updater)
{
  // check if the Updaters is part of the list.
  Updaters::iterator iter = updaters_.find(const_cast<Updater*>(updater));
  if (iter == updaters_.end()) {
    return;
  }

  // remove the element
  updaters_.erase (iter);
}

void
UpdateManager::shutdown (void)
{
}

int
UpdateManager::init (int , ACE_TCHAR *[])
{
  ACE_DEBUG ((LM_DEBUG, "UpdateManager::init"));

  return 0;
}

int
UpdateManager::fini (void)
{
  return 0;
}

void
UpdateManager::unregisterCallback (void)
{
  // TBD
}

void
UpdateManager::requestImage (void)
{
  // TBD
}

void
UpdateManager::add (const UTopic& topic)
{
  if (updaters_.empty()) {
    return;
  }

  // serialize the Topic QOS
  TAO_OutputCDR outCdr;
  outCdr << topic.topicQos;

  size_t len = outCdr.total_length();
  BinStr qos_bin (len);
  const char *buf = outCdr.buffer();
  for (size_t count = 0; count < len; count++) {
    qos_bin [count] = buf [count];
  }

  QosType p (Topic, qos_bin);
  TopicData topic_data = {topic.domainId, topic.topicId, topic.particiapntId
                          , topic.name, topic.dataType, p};

  // Invoke add on each of the iterators.
  for (Updaters::iterator iter = updaters_.begin();
       iter != updaters_.end();
       iter++) {
    (*iter)->add (topic_data);
  }
}

void
UpdateManager::add(const ParticipantData& participant)
{
  // Invoke add on each of the iterators.
  for (Updaters::iterator iter = updaters_.begin();
       iter != updaters_.end();
       iter++) {
    (*iter)->add (participant);
  }
}

void
UpdateManager::add(const ActorData& actor)
{
  // Invoke add on each of the iterators.
  for (Updaters::iterator iter = updaters_.begin();
       iter != updaters_.end();
       iter++) {
    (*iter)->add (actor);
  }
}

void
UpdateManager::remove(const ItemType& itemType, const IdType& id)
{
  // Invoke remove on each of the iterators.
  for (Updaters::iterator iter = updaters_.begin();
       iter != updaters_.end();
       iter++) {
    (*iter)->remove (itemType, id);
  }
}

void
UpdateManager::updateQos(const ItemType& itemType, const IdType& id
			 , const QosType& qos)
{
  // Invoke updateQos on each of the iterators.
  for (Updaters::iterator iter = updaters_.begin();
       iter != updaters_.end();
       iter++) {
    (*iter)->updateQos (itemType, id, qos);
  }
}

int
UpdateManager_Loader::init (void)
{
  //return ACE_Service_Config::process_directive
  //(ace_svc_desc_UpdateManager);
  return 0;
}

ACE_FACTORY_DEFINE (ACE_Local_Service, UpdateManager)

ACE_STATIC_SVC_DEFINE (UpdateManager,
                         ACE_TEXT ("UpdateManager"),
                         ACE_SVC_OBJ_T,
                         &ACE_SVC_NAME (UpdateManager),
                         ACE_Service_Type::DELETE_THIS
                         | ACE_Service_Type::DELETE_OBJ,
                         0)

ACE_STATIC_SVC_REQUIRE (UpdateManager)
