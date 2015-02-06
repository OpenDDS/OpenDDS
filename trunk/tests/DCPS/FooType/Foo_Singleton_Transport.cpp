

#include "Foo_Singleton_Transport.h"

Singleton_Transport::Singleton_Transport(void)
  : current_foo_()
{
}


Singleton_Transport::~Singleton_Transport(void)
{
}

::DDS::ReturnCode_t Singleton_Transport::set_foo (const Foo& foo)
{
  current_foo_ = foo;
  return ::DDS::RETCODE_OK;
}


Foo Singleton_Transport::get_foo ()
{
  Foo retVal = current_foo_;
  return retVal;
}


