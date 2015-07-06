/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "UpdateManager.h"

template<class UType>
void
Update::Manager::create(const UType& info)
{
  // Invoke add on each of the iterators.
  for (Updaters::iterator iter = updaters_.begin();
       iter != updaters_.end();
       iter++) {
    (*iter)->create(info);
  }
}

template<class QosType>
void
Update::Manager::update(const Update::IdPath& id, const QosType& qos)
{
  // Invoke update on each of the iterators.
  for (Updaters::iterator iter = updaters_.begin();
       iter != updaters_.end();
       iter++) {
    (*iter)->update(id, qos);
  }
}
