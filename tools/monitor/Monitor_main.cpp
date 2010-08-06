/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Viewer.h"
#include "Options.h"
#include <QtGui/QSplashScreen>

#include "dds/DCPS/Service_Participant.h"

int
main( int argc, char** argv)
{
  // Initialize the application, consume any Qt arguments.
  QApplication  application( argc, argv);
  QPixmap       splashImage(":/jpeg/splash.jpg");
  QSplashScreen splash(splashImage);
  splash.show();
  application.processEvents();

  // Initialize the service and consume the ACE+TAO+DDS arguments.
  TheParticipantFactoryWithArgs( argc, argv);
  application.processEvents();

  // Load the SimpleTcp transport library as we know that we will be
  // using it.
  ACE_Service_Config::process_directive(
    ACE_TEXT("dynamic DCPS_SimpleTcpLoader Service_Object * ")
    ACE_TEXT("SimpleTcp:_make_DCPS_SimpleTcpLoader() \"-type SimpleTcp\"")
  );
  application.processEvents();

  // Process the command line arguments left after ACE and Qt have had a go.
  Monitor::Options options( argc, argv);
  application.processEvents();

  // Instantiate and display.
  Monitor::Viewer* viewer = new Monitor::Viewer( options);
  viewer->show();
  splash.finish( viewer);

  // Main GUI processing loop.
  return application.exec();
}

