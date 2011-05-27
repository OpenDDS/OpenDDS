#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "PublisherFactory.h"

namespace
{
  TAO::DCPS::PublisherFactory*& thePublisherFactory()
  {
    static TAO::DCPS::PublisherFactory* thePublisherFactory = 0;
    return thePublisherFactory;
  }
}

TAO::DCPS::PublisherFactory&
TAO::DCPS::getPublisherFactory()
{
  return *thePublisherFactory();
}

void
TAO::DCPS::setPublisherFactory(TAO::DCPS::PublisherFactory* factory)
{
  thePublisherFactory() = factory;
}
