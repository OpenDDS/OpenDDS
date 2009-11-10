/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Viewer.h"

#include "dds/DCPS/Service_Participant.h"

int
main( int argc, char** argv, char**)
{
  // Initialize the service and consume the ACE+TAO+DDS arguments.
  ::DDS::DomainParticipantFactory_var factory
    = TheParticipantFactoryWithArgs( argc, argv);

  // Initialize the application.
  QApplication application( argc, argv);

  // Instantiate and display.
  Monitor::Viewer* viewer = new Monitor::Viewer();
  viewer->show();

  // Main GUI processing loop.
  return application.exec();
}

