/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef UPDATE_MANAGER_H
#define UPDATE_MANAGER_H

#include "inforepo_export.h"
#include "UpdateDataTypes.h"
#include "Updater.h"

#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DdsDcpsInfoUtilsC.h"

#include "tao/CDR.h"

#include "ace/Service_Object.h"
#include "ace/Service_Config.h"

#include <set>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

// forward declarations
class TAO_DDS_DCPSInfo_i;

namespace Update {

class OpenDDS_InfoRepoLib_Export Manager : public ACE_Service_Object {
public:
  Manager();

  virtual ~Manager();

  /// Shared object initializer
  virtual int init(int argc, ACE_TCHAR *argv[]);

  /// Shared object finalizer
  virtual int fini();

  // mechanism for InfoRepo object to be registered.
  void add(TAO_DDS_DCPSInfo_i* info);
  void add(Updater* updater);

  // Mechanism to unregister Updaters/InfoRepo
  void remove();
  void remove(const Updater* updater);

  /// Force a clean shutdown.
  // void shutdown (void);

  /// Upstream request for a fresh image
  /// Currently handled synchronously via 'pushImage'
  /// TBD: Replace with an asynchronous model.
  void requestImage();

  /// Downstream request to push image
  void pushImage(const DImage& image);

  // Propagate creation of entities.
  template<class UType>
  void create(const UType& info);

  // Propagate QoS updates.
  template<class QosType>
  void update(const IdPath& id, const QosType& qos);

  // Propagate destruction of entities.
  void destroy(const IdPath& id, ItemType type, ActorType actor = DataWriter);

  // Downstream request to push persisted data
  void add(const DTopic& topic);
  void add(const DParticipant& participant);
  void add(const DActor& actor);

private:
  typedef std::set <Updater*> Updaters;

  // required to break an include dependency loop
  //void add (Updater* updater, const DActor& actor);

  TAO_DDS_DCPSInfo_i* info_;
  Updaters updaters_;
};

} // End of namespace Update

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (ACE_TEMPLATES_REQUIRE_SOURCE)
#include "UpdateManager_T.cpp"
#endif /* ACE_TEMPLATES_REQUIRE_SOURCE */

#if defined (ACE_TEMPLATES_REQUIRE_PRAGMA)
#pragma message ("UpdateManager_T.cpp template inst")
#pragma implementation ("UpdateManager_T.cpp")
#endif /* ACE_TEMPLATES_REQUIRE_PRAGMA */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

typedef Update::Manager UpdateManagerSvc;

ACE_STATIC_SVC_DECLARE(UpdateManagerSvc)

ACE_FACTORY_DECLARE(ACE_Local_Service, UpdateManagerSvc)

class OpenDDS_InfoRepoLib_Export UpdateManagerSvc_Loader {
public:
  static int init();
};

#if defined(ACE_HAS_BROKEN_STATIC_CONSTRUCTORS)

typedef int (*UpdateManagerSvc_Loader)();

static UpdateManagerSvc_Loader ldr =
  &UpdateManagerSvc_Loader::init;

#else

static int ldr =
  UpdateManagerSvc_Loader::init();

#endif /* ACE_HAS_BROKEN_STATIC_CONSTRUCTORS */

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* UPDATE_MANAGER_H */
