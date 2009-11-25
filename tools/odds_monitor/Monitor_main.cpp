/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Viewer.h"
#include "Options.h"

#include "dds/DCPS/Service_Participant.h"

int
main( int argc, char** argv, char**)
{
  // Initialize the service and consume the ACE+TAO+DDS arguments.
  TheParticipantFactoryWithArgs( argc, argv);

  // Initialize the application, consume any Qt arguments.
  QApplication application( argc, argv);

  // Process the command line arguments left after ACE and Qt have had a go.
  Monitor::Options options( argc, argv);

  // Instantiate and display.
  Monitor::Viewer* viewer = new Monitor::Viewer( options);
  viewer->show();

  // Main GUI processing loop.
  return application.exec();
}

