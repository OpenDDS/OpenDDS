/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Viewer.h"
#include "Options.h"
#include <QtGui/QSplashScreen>

#include "dds/DCPS/Service_Participant.h"
#include <ace/Argv_Type_Converter.h>

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  // Initialize the application, consume any Qt arguments.
  ACE_Argv_Type_Converter conv(argc, argv);
  QApplication  application(conv.get_argc(), conv.get_ASCII_argv());
  QPixmap       splashImage(":/jpeg/splash.jpg");
  QSplashScreen splash(splashImage);
  splash.show();
  application.processEvents();

  // Initialize the service and consume the ACE+TAO+DDS arguments.
  TheParticipantFactoryWithArgs(argc, argv);
  application.processEvents();

  // Load the Tcp transport library as we know that we will be
  // using it.
  ACE_Service_Config::process_directive(
    ACE_TEXT("dynamic OpenDDS_Tcp Service_Object * ")
    ACE_TEXT("OpenDDS_Tcp:_make_TcpLoader()")
  );
  application.processEvents();

  // Process the command line arguments left after ACE and Qt have had a go.
  Monitor::Options options(argc, argv);
  application.processEvents();

  // Instantiate and display.
  Monitor::Viewer* viewer = new Monitor::Viewer(options);
  viewer->show();
  splash.finish(viewer);

  // Main GUI processing loop.
  return application.exec();
}

