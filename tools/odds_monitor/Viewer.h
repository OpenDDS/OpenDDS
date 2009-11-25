/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
#ifndef VIEWER_H
#define VIEWER_H

#include "ui_Monitor.h"

namespace Monitor {

class Options;
class MonitorData;
class MonitorDataModel;

class Viewer : public QMainWindow {
  Q_OBJECT

  public:
    Viewer( const Options& options, QMainWindow* parent = 0);

  private slots:
    void addRepo();
    void removeRepo();
    void newRepo( const QString& ior);

  protected:
    void closeEvent( QCloseEvent* event = 0);

void stubmodelchange();

  private:
    Ui::Monitor          ui;
    const Options&       options_;
    MonitorData*         dataSource_;
    MonitorDataModel*    model_;
};

} // End of namespace Monitor

#endif /* VIEWER_H */

