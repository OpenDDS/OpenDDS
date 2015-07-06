// ============================================================================
/**
 *  @file   Foo_Singleton_Transport.h
 *
 *
 *
 */
// ============================================================================

#ifndef FOO_SINGLETON_TRANSPORT_H_
#define FOO_SINGLETON_TRANSPORT_H_

#include "footype_export.h"
#include "FooTypeC.h"

#include "ace/Singleton.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

/**
 * @class Singleton_Transport
 *
 * @brief A simple transport for the Foo type
 *
 */
class FooType_Export Singleton_Transport
{
  friend class ACE_Singleton<Singleton_Transport, ACE_SYNCH_MUTEX>;

public:

  /// Sets the current foo
  ::DDS::ReturnCode_t set_foo (const Foo& foo);

  /// reads the current foo
  Foo get_foo ();

private:
  Singleton_Transport(void);
  ~Singleton_Transport(void);

  // this could be expanded into a sequence.
  Foo current_foo_;

};

typedef ACE_Singleton<Singleton_Transport, ACE_SYNCH_MUTEX> FOO_SINGLETON_TRANSPORT;

FOOTYPE_SINGLETON_DECLARE(ACE_Singleton, Singleton_Transport, ACE_SYNCH_MUTEX);

#define Foo_Singleton_Transport FOO_SINGLETON_TRANSPORT::instance()

#endif /* FOO_SINGLETON_TRANSPORT_H_  */
