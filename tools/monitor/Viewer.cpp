/*
*
*
* Distributed under the OpenDDS License.
* See: http://www.opendds.org/license.html
*/

#include <QtGui/QtGui>
#include <QtGui/QPixmap>
#include <QtGui/QScrollBar>
#include <QtGui/QGraphicsScene>
#include <sstream>
#include "Viewer.h"
#include "RepoSelect.h"
#include "Options.h"
#include "MonitorData.h"
#include "MonitorDataModel.h"
#include "TreeNode.h"
#include "GraphGenerator.h"
#include "NodeGenerator.h"

#include "dds/DCPS/Service_Participant.h"

#ifdef DEVELOPMENT
#include <iostream>
#endif /* DEVELOPMENT */

namespace { // Anonymous namespace for file scope.
  /// Default (detached) repository selection item label.
  const QString repoDetachedSelection( QObject::tr("<detached>"));

} // End of anonymous namespace.

namespace Monitor {

  Viewer::Viewer( const Options& options, QMainWindow* parent)
  : QMainWindow( parent),
  options_( options),
  dataSource_( 0),
  model_( 0),
  graphView(0),
  image(0),
  nodeScene(0)
  {
    // Initialize the GUI and connect the signals and slots.
    this->ui.setupUi( this);

    // no rendering artifacts
    this->ui.nodeView->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    // context menu creation
    treeMenu.setTitle("Tree View");
    treeMenu.addAction(EXPAND_ACTION);
    treeMenu.addAction(SELECTION_TO_NODE_VIEW);
    treeMenu.addAction(SELECTION_TO_GRAPH_VIEW);
    treeMenu.addAction(NODE_VIEW_OPTIONS_ACTION);
    treeMenu.addAction(GRAPH_OPTIONS_ACTION);

    nodeViewMenu.setTitle("Node View");
    nodeViewMenu.addAction(NODE_VIEW_UPDATE_ACTION);
    nodeViewMenu.addAction(NODE_VIEW_OPTIONS_ACTION);
    nodeViewMenu.addAction(NODE_VIEW_TO_FILE);

    nodeMenu.setTitle("Node Options");
    // TODO: implement node options
    //nodeMenu.addAction(NODE_EXPAND);
    //nodeMenu.addAction(NODE_COLLAPSE);
    //nodeMenu.addAction(NODE_DELETE);
    //nodeMenu.addAction(NODE_CONNECT);
    //nodeMenu.addAction(NODE_DISCONNECT);
    //nodeMenu.addAction(NODE_RESTORE_CONNECTIONS);

    graphMenu.setTitle("Graph View");
    graphMenu.addAction(GRAPH_UPDATE_ACTION);
    graphMenu.addAction(GRAPH_OPTIONS_ACTION);
    graphMenu.addAction(GRAPH_VIEW_TO_FILE);

    // context menu connections
    this->ui.repoView->setContextMenuPolicy(Qt::CustomContextMenu);
    this->ui.nodeView->setContextMenuPolicy(Qt::CustomContextMenu);
    this->ui.scrollArea->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(this->ui.repoView, SIGNAL(customContextMenuRequested(const QPoint&)),
      this, SLOT(treeContextMenu(const QPoint&)));
    connect(this->ui.nodeView, SIGNAL(customContextMenuRequested(const QPoint&)),
      this, SLOT(nodeContextMenu(const QPoint&)));
    connect(this->ui.scrollArea, SIGNAL(customContextMenuRequested(const QPoint&)),
      this, SLOT(graphContextMenu(const QPoint&)));

    //
    // SIGNAL to SLOT connections (GUI to behavior connections).
    //

    // Add a repository from button.
    connect( this->ui.repoAddButton,    SIGNAL(clicked()), this, SLOT(addRepo()));

    // Remove a repository from button.
    connect( this->ui.repoRemoveButton, SIGNAL(clicked()), this, SLOT(removeRepo()));

    // Connect Graphviz scale slider
    connect( this->ui.horizontalSlider, SIGNAL(sliderReleased()), this, SLOT(scaleImage()));

    // Quit from button.
    connect( this->ui.quitButton,       SIGNAL(clicked()), this, SLOT(close()));

    // Combo box selection change attaches to different repository.
    connect( this->ui.repoSelection, SIGNAL(currentIndexChanged( const QString&)),
      this,                   SLOT(newRepo( const QString&)));

    // Clicking on header sorts column.
    connect( this->ui.repoView->header(), SIGNAL(sectionClicked(int)),
      this,                        SLOT(doSort(int)));

    // Expanding tree node resizes column
    connect( this->ui.repoView, SIGNAL(expanded(const QModelIndex&)),
      this,              SLOT(itemExpanded(const QModelIndex&)));

    // Sync GvOptionsData to GvDialog
    GvOptionsDialog::getGvData(this, gvOpt);

    // Sync nodeOptions
    NodeOptionsDialog::getNodeData(this, nodeOpt);

    // GUI to behavior attachments complete.

    // Initialize the model and tree view.
    this->model_      = new MonitorDataModel(this->ui.repoView);
    this->dataSource_ = new MonitorData( this->options_, this->model_);
    this->ui.repoView->setModel( this->model_);

    // need multi select for sending to node/graph views
    this->ui.repoView->setSelectionMode(QTreeView::ExtendedSelection);
    this->ui.repoView->setSelectionBehavior(QTreeView::SelectRows);

    // Resize columns on dataChanged(). has to happen after we have a model_.
    connect( this->model_,
      SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
      this,
      SLOT(resizeColumnsOnDataChange(const QModelIndex &, const QModelIndex &)));


    // Setup the repository selections using the initial set.
    this->ui.repoSelection->addItem( repoDetachedSelection);
    QList<QString> iorList;
    this->dataSource_->getIorList( iorList);
    while( !iorList.isEmpty()) {
      this->ui.repoSelection->addItem( iorList.takeFirst());
    }

    // Establish the most recent selection as the initial one.
    int count = this->ui.repoSelection->count();
    if( count > 0) {
      this->ui.repoSelection->setCurrentIndex( count - 1);
    }
  }

  void
  Viewer::itemExpanded( const QModelIndex& item)
  {
    this->ui.repoView->resizeColumnToContents( item.column());
  }

  void
  Viewer::resizeColumnsOnDataChange(const QModelIndex & start,
    const QModelIndex & end)
  {
    for( int column = start.column(); column <= end.column(); ++column) {
      this->ui.repoView->resizeColumnToContents( column);
    }
  }

  void
  Viewer::addRepo()
  {
    // Query the user for a repository IOR value.
    bool status = false;
    QString iorString = RepoSelect::getRepoIOR( this, &status);
    if( status && !iorString.isEmpty()) {
      // Don't add the default detached value accidentally.
      if( iorString == repoDetachedSelection) {
        this->ui.statusbar->showMessage( tr("Invalid IOR value"));
        return;
      }

      // Add the value as a selection.
      this->ui.repoSelection->addItem( iorString);

      //// Setting the index results in the signal calling newRepo().
      //int index = this->ui.repoSelection->findText( iorString);
      //this->ui.repoSelection->setCurrentIndex( index);
      // Status bar updated when the index change propagates.

    } else {
      this->ui.statusbar->showMessage( tr("Detached"));
    }
  }

  void
  Viewer::removeRepo()
  {
    QString current = this->ui.repoSelection->currentText();
    if( current != repoDetachedSelection) {
      this->dataSource_->removeRepo( current);
      this->ui.repoSelection->removeItem( this->ui.repoSelection->currentIndex());
    }
    // Status bar is updated when the index change propagates.
  }

  void
  Viewer::scaleImage()
  {
    std::stringstream msg;

    if (!graphView) {
      msg << "No image to scale";

    }
    else {
      // scale image by amount on slider
      double factor = (100.0 - this->ui.horizontalSlider->value()) / 100.0;
      graphView->resize(factor * graphView->pixmap()->size());

      QScrollBar* scrollBar = this->ui.scrollArea->horizontalScrollBar();

      scrollBar->setValue(int(factor * scrollBar->value()
        + ((factor - 1) * scrollBar->pageStep()/2)));

      scrollBar = this->ui.scrollArea->verticalScrollBar();

      scrollBar->setValue(int(factor * scrollBar->value()
        + ((factor - 1) * scrollBar->pageStep()/2)));

      this->ui.scrollArea->setBackgroundRole(QPalette::Dark);
      this->ui.scrollArea->setWidget(graphView);
      this->ui.scrollArea->show();


      msg << "Image scaled to " << factor;
    }

    this->ui.statusbar->showMessage( tr(msg.str().c_str()));

  }


  void
  Viewer::newRepo( const QString& ior)
  {
    if( ior != repoDetachedSelection) {
      if( !ior.isEmpty()) {
        if( this->dataSource_->setRepoIor( ior)) {
          this->ui.statusbar->showMessage( tr("Attached"));

        } else {
          // This selection was not a valid repository, remove it from the
          // selection list (since it was already added to the list to get
          // to this point).
          this->removeRepo();
          this->ui.statusbar->showMessage( tr("Failed to attach"));
        }
      }
    } else {
      // This is the <detached> selection, remove any active repository.
      this->dataSource_->stopInstrumentation();
      this->dataSource_->clearData();
      this->ui.statusbar->showMessage( tr("Detached"));
    }
  }

  void
  Viewer::doSort( int index)
  {
    Qt::SortOrder ordering = this->ui.repoView->header()->sortIndicatorOrder();
    #ifdef DEVELOPMENT
    std::cerr << "Sorting column " << index << " ordered by " << ordering << std::endl;
    #endif /* DEVELOPMENT */
    this->ui.repoView->sortByColumn( index, ordering);
  }

  void
  Viewer::treeContextMenu(const QPoint& pos)
  {
    QPoint globalPos = this->ui.repoView->viewport()->mapToGlobal(pos);

    QAction* selectedItem = treeMenu.exec(globalPos);
    if (selectedItem) {
      if (selectedItem->iconText() == EXPAND_ACTION)
      {
        expand(true);
        selectedItem->setText(COLLAPSE_ACTION);
        selectedItem->setIconText(COLLAPSE_ACTION);

      }
      else if (selectedItem->iconText() == COLLAPSE_ACTION)
      {
        expand(false);
        selectedItem->setText(EXPAND_ACTION);
        selectedItem->setIconText(EXPAND_ACTION);
      }
      else if (selectedItem->iconText() == SELECTION_TO_NODE_VIEW ||
        selectedItem->iconText() == SELECTION_TO_GRAPH_VIEW)
      {
        // reset all the display flags
        this->dataSource_->modelRoot()->resetDisplays();

        QModelIndexList q = this->ui.repoView->selectionModel()->selectedRows();
        for (int c = 0; c < q.size(); ++c) {
          // get the node and set the display flag
          TreeNode *t = model_->getNode(q[c]);
          t->setDisplay(true);
        }

        if (selectedItem->iconText() == SELECTION_TO_NODE_VIEW)
        {
          updateNodeView(true);
        }
        else {
          updateGraphView(true);
        }
      }
      else if (selectedItem->iconText() == NODE_VIEW_OPTIONS_ACTION)
      {
        showNodeOptions();
      }
      else if (selectedItem->iconText() == GRAPH_OPTIONS_ACTION)
      {
        showGraphOptions();
      }
    }
  }

  void
  Viewer::nodeContextMenu(const QPoint& pos)
  {
    bool clickedOnItem = (this->ui.nodeView->itemAt(pos) != 0);

    QPoint globalPos = this->ui.nodeView->mapToGlobal(pos);

    QAction* selectedItem;

    if (clickedOnItem) {
      selectedItem = nodeMenu.exec(globalPos);
    }
    else {
      selectedItem = nodeViewMenu.exec(globalPos);
    }

    if (selectedItem) {
      if (selectedItem->iconText() == NODE_VIEW_UPDATE_ACTION)
      {
        updateNodeView();
      }
      else if (selectedItem->iconText() == NODE_VIEW_OPTIONS_ACTION)
      {
        showNodeOptions();
      }
      else if (selectedItem->iconText() == NODE_VIEW_TO_FILE)
      {
        if (nodeScene != 0) {
          // create a pixmap
          QPixmap p(nodeScene->width(), nodeScene->height());
          p.fill(); // with white
          QPainter painter(&p);
          painter.setBackgroundMode(Qt::OpaqueMode); // must set opaque otherwise black background generated
          this->ui.nodeView->render(&painter);
          saveImage(&p);
        }
        else {
          this->ui.statusbar->showMessage( tr("No image to save."));
        }
      }
    }
  }

  void
  Viewer::graphContextMenu(const QPoint& pos)
  {
    QPoint globalPos = this->ui.scrollArea->mapToGlobal(pos);

    QAction* selectedItem = graphMenu.exec(globalPos);
    if (selectedItem) {
      if (selectedItem->iconText() == GRAPH_UPDATE_ACTION)
      {
        updateGraphView();
      }
      else if (selectedItem->iconText() == GRAPH_OPTIONS_ACTION)
      {
        showGraphOptions();
      }
      else if (selectedItem->iconText() == GRAPH_VIEW_TO_FILE)
      {
        if (graphView != 0) {
          saveImage(graphView->pixmap());
        }
        else {
          this->ui.statusbar->showMessage( tr("No image to save."));
        }
      }
    }
  }

  void
  Viewer::showGraphOptions()
  {
    // Query the user for Graphviz output filenames.
    bool status = false;
    GvOptionsDialog::dialogAction( this, gvOpt, &status);

  }

  void
  Viewer::updateGraphView(bool honorDisplayFlag)
  {
    try {
      // invalid QModelIndex signifies the root.
      GraphGenerator g(this->dataSource_->modelRoot(), gvOpt, honorDisplayFlag);

      // this creates dot file and png file
      g.draw();

      std::stringstream msg;

      if (image != 0) {
        delete image;
        image = 0;
      }

      image = new QImage(gvOpt.getPng().c_str());

      if (image != 0) {
        // setWidget calls delete on old graphview. no need to delete.
        graphView = new QLabel();

        graphView->setBackgroundRole(QPalette::Base);
        graphView->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        graphView->setScaledContents(true);

        QPixmap pixmap = QPixmap::fromImage(*image);

        // scale image by amount on slider
        double factor = (100.0 - this->ui.horizontalSlider->value()) / 100.0;

        graphView->setPixmap(pixmap);

        graphView->resize(factor * graphView->pixmap()->size());

        QScrollBar* scrollBar = this->ui.scrollArea->horizontalScrollBar();

        scrollBar->setValue(int(factor * scrollBar->value()
          + ((factor - 1) * scrollBar->pageStep()/2)));

        scrollBar = this->ui.scrollArea->verticalScrollBar();

        scrollBar->setValue(int(factor * scrollBar->value()
          + ((factor - 1) * scrollBar->pageStep()/2)));

        this->ui.scrollArea->setBackgroundRole(QPalette::Dark);
        this->ui.scrollArea->setWidget(graphView);
        this->ui.scrollArea->show();

        msg << "Graph View Updated " << image->size().width() << "x" << image->size().height();

        this->ui.statusbar->showMessage( tr(msg.str().c_str()));
      }
      else {
        msg << "Error loading image file";
        this->ui.statusbar->showMessage( tr(msg.str().c_str()));
      }
    }
    catch (const char *&e) {
      this->ui.statusbar->showMessage(tr(e));
    }

  }

  void
  Viewer::showNodeOptions()
  {
    bool status = false;
    NodeOptionsDialog::dialogAction( this, nodeOpt, &status);
  }

  void
  Viewer::updateNodeView(bool honorDisplayFlag)
  {
    if (nodeScene != 0) {
      delete nodeScene;
      nodeScene = 0;
    }

    // invalid QModelIndex signifies the root.
    NodeGenerator ng(this->dataSource_->modelRoot(), nodeOpt, honorDisplayFlag);

    // calculate the layout based on options and number of nodes
    ng.calcLayout(this->dataSource_->modelRoot(), 1, 1);

    nodeScene = new QGraphicsScene(0, 0, ng.getSceneWidth(), ng.getSceneHeight());
    this->ui.nodeView->setScene(nodeScene);

    ng.draw(nodeScene);

    std::stringstream msg;
    msg << "Node View Updated " << ng.getSceneWidth() << "x" << ng.getSceneHeight();

    this->ui.statusbar->showMessage( tr(msg.str().c_str()));
  }

  void
  Viewer::expand(bool flag)
  {
    if (flag) {
      this->ui.repoView->expandAll();
    }
    else {
      this->ui.repoView->collapseAll();
    }
  }

  void
  Viewer::saveImage(const QPixmap *p)
  {
    if (!p) {
      return;
    }

    QFileDialog fd(0, tr("Save Image"), QString(),
      tr("Image Files (*.png *.bmp *.jpg *.jpeg)"));
    fd.setDefaultSuffix("png");
    fd.setAcceptMode(QFileDialog::AcceptSave);
    fd.exec();

    if(!fd.selectedFiles().isEmpty()) {
      p->save(fd.selectedFiles().front());
      this->ui.statusbar->showMessage(tr("Image saved."));
    }
    else {
      this->ui.statusbar->showMessage(tr("Image not saved."));
    }
  }

  void
  Viewer::closeEvent( QCloseEvent* /* event */)
  {
    this->ui.statusbar->showMessage( tr("Closing"));

    this->dataSource_->disable();
    delete this->model_;
    delete this->dataSource_;

    ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) Viewer::closeEvent() - cleaning up service resources")
    ));

    // Clean up the service resources.
    TheServiceParticipant->shutdown();
  }

} // End of namespace Monitor
