// -*- C++ -*-
// $Id$

#include "UpdateManager.h"

template< class UType>
void
Update::UpdateManager::create( const UType& info)
{
  // Invoke add on each of the iterators.
  for (Updaters::iterator iter = updaters_.begin();
       iter != updaters_.end();
       iter++) {
    (*iter)->create( info);
  }
}

template< class QosType>
void
Update::UpdateManager::update( const Update::IdType& id, const QosType& qos)
{
  // Invoke update on each of the iterators.
  for (Updaters::iterator iter = updaters_.begin();
       iter != updaters_.end();
       iter++) {
    (*iter)->update( id, qos);
  }
}

