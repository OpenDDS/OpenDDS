/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef VIEWER_H
#define VIEWER_H

#include "ui_Monitor.h"
#include "GvOptions.h"
#include "NodeOptions.h"

#include <QtGui/QScrollArea>

// context menu items
#define EXPAND_ACTION "Expand All"
#define COLLAPSE_ACTION "Collapse All"
#define SELECTION_TO_NODE_VIEW "Selection to Node View"
#define SELECTION_TO_GRAPH_VIEW "Selection to Graph View"

#define NODE_VIEW_UPDATE_ACTION "Update Node View"
#define NODE_VIEW_OPTIONS_ACTION "Show Node Options"
#define NODE_VIEW_TO_FILE "Save Node View to File"

#define NODE_EXPAND "Expand Node"
#define NODE_COLLAPSE "Collapse Node"
#define NODE_DELETE "Delete Node"
#define NODE_CONNECT "Connect"
#define NODE_DISCONNECT "Disconnect"
#define NODE_RESTORE_CONNECTIONS "Restore Connections"

#define GRAPH_UPDATE_ACTION "Update Graph View"
#define GRAPH_OPTIONS_ACTION "Show Graph Options"
#define GRAPH_VIEW_TO_FILE "Save Graph View to File"

namespace Monitor {

class Options;
class MonitorData;
class MonitorDataModel;

class Viewer : public QMainWindow {
  Q_OBJECT

  public:
    Viewer( const Options& options, QMainWindow* parent = 0);

  private slots:
    /// Translate index to column to signal the view to resize the column.
    void itemExpanded( const QModelIndex& item);

    /// Data has changed. Moved from MonitorDataModel to avoid calling
    /// outside of GUI thread. This fixes the "QPixmap: It is not safe to
    /// use pixmaps outside the GUI thread" warnings.
    void resizeColumnsOnDataChange(const QModelIndex & start,
                                   const QModelIndex & end);

    void addRepo();
    void removeRepo();
    void scaleImage();
    void newRepo( const QString& ior);
    void doSort( int index);

    // context menu slots
    void treeContextMenu(const QPoint&);
    void nodeContextMenu(const QPoint&);
    void graphContextMenu(const QPoint&);

    // methods called from context menus
    void showGraphOptions();
    void updateGraphView(bool honorDisplayFlag = false);
    void showNodeOptions();
    void updateNodeView(bool honorDisplayFlag = false);
    void expand(bool flag = false);
    void saveImage(const QPixmap *p);

  protected:
    /// Close action.
    void closeEvent( QCloseEvent* event = 0);

  private:
    Ui::Monitor          ui;
    const Options&       options_;
    MonitorData*         dataSource_;
    MonitorDataModel*    model_;

    GvOptionsData gvOpt;  // the authority for gvoptions ie the gvdialog may
                          // not have the correct options if it's hidden.

    NodeOptionsData nodeOpt;

    // these items are not created with the designer tool
    // they belong to the viewer not the ui.
    QLabel* graphView;
    QImage* image;

    // rt-click context menu popups for tree, node and graph view options
    QMenu treeMenu;
    QMenu nodeViewMenu; // node view options
    QMenu nodeMenu; // individual node options
    QMenu graphMenu;

    QGraphicsScene *nodeScene;
};

} // End of namespace Monitor

#endif /* VIEWER_H */

