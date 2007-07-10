// -*- C++ -*-

/**
 * @file      UpdateManager.h
 *
 * library   UpdateManager
 *
 * $Id$
 *
 * @author Ciju John <johnc@ociweb.com>
 */


#ifndef _UPDATE_MANAGER_
#define _UPDATE_MANAGER_

#include "UpdateDataTypes.h"

#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DdsDcpsInfoUtilsC.h"

#include "tao/CDR.h"

#include "ace/Service_Object.h"
#include "ace/Service_Config.h"

#include <set>

// forward declarations
class TAO_DDS_DCPSInfo_i;
class Updater;

class UpdateManager : public ACE_Service_Object
{
 public:
  typedef struct TopicStrt< ::DDS::TopicQos &, std::string> UTopic; // U == upstream
  typedef struct ParticipantStrt< ::DDS::DomainParticipantQos &> UParticipant;
  typedef struct ActorStrt< ::DDS::PublisherQos &
                            , ::DDS::DataWriterQos &
                            , std::string
                            , OpenDDS::DCPS::TransportInterfaceInfo &> UWActor;
  typedef struct ActorStrt< ::DDS::SubscriberQos &
                            , ::DDS::DataReaderQos &
                            , std::string
                            , OpenDDS::DCPS::TransportInterfaceInfo &> URActor;
  typedef struct ImageData<UTopic*, UParticipant*, URActor*, UWActor*> UImage;

  // ciju: The below aggregative structures should probably be using
  // refreences rather than real object.
  // TBD: Make the above mentioned changes.
  typedef struct TopicStrt<QosSeq, std::string> DTopic; // D == downstream
  typedef struct ParticipantStrt<QosSeq> DParticipant;
  typedef struct ActorStrt<QosSeq, QosSeq, std::string, BinSeq> DActor;

  typedef struct ImageData<DTopic, DParticipant, DActor, DActor> DImage;

  UpdateManager (void);

  virtual ~UpdateManager (void);

  /// Shared object initializer
  virtual int init (int argc, ACE_TCHAR *argv[]);

  /// Shared object finalizer
  virtual int fini (void);


  // mechanism for InfoRepo object to be registered.
  void add (TAO_DDS_DCPSInfo_i* info);
  void add (Updater* updater);

  // Mechanism to unregister Updaters/InfoRepo
  void remove ();
  void remove (const Updater* updater);

  /// Force a clean shutdown.
  // void shutdown (void);

  // void unregisterCallback (void);

  /// Upstream request for a fresh image
  /// Currently handled synchronously via 'pushImage'
  /// TBD: Replace with an asynchronous model.
  void requestImage (void);

  /// Downstream request to push image
  void pushImage (const DImage& image);

  /// Upstream request to persist data.
  void add (const UTopic& topic);
  void add (const UParticipant& participant);

  template <typename UA>
  void add (const UA& actor);

  /// Upstream request to remove entries
  // Remove an entity (but not children) from persistence.
  void remove (ItemType type, const IdType& id);

  // Persist updated Qos parameters for an entity.
  void updateQos(const ItemType& itemType, const IdType& id
			 , const QosSeq& qos);

  // Downstream request to push persisted data
  void add (const DTopic& topic);
  void add (const DParticipant& participant);
  void add (const DActor& actor);

 private:
  typedef std::set <Updater*> Updaters;

  // required to break an include dependency loop
  void add (Updater* updater, const DActor& actor);

  TAO_DDS_DCPSInfo_i* info_;
  Updaters updaters_;
};

#include "UpdateManager.inl"

ACE_STATIC_SVC_DECLARE (UpdateManager)

ACE_FACTORY_DECLARE (ACE_Local_Service, UpdateManager)

class UpdateManager_Loader
{
public:
  static int init (void);
};

#if defined(ACE_HAS_BROKEN_STATIC_CONSTRUCTORS)

typedef int (*UpdateManager_Loader) (void);

static UpdateManager_Loader ldr =
&UpdateManager_Loader::init;

#else

static int ldr =
UpdateManager_Loader::init ();

#endif /* ACE_HAS_BROKEN_STATIC_CONSTRUCTORS */

#endif // _UPDATE_MANAGER_
