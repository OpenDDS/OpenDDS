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

class MonitorDataModel;
class TreeNode;

class Viewer : public QMainWindow {
  Q_OBJECT

  public:
    Viewer( QMainWindow* parent = 0);

  private slots:
    void openFile();
    void getIor();
    void modifyTree();

  protected:
    void closeEvent( QCloseEvent* event = 0);

  private:
    Ui::Monitor       ui;
    MonitorDataModel* model_;
    TreeNode*         root_;
};

} // End of namespace Monitor

#endif /* VIEWER_H */

