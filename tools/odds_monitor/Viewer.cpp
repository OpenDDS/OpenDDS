/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <QtGui/QtGui>
#include <sstream>
#include "Viewer.h"
#include "RepoSelect.h"
#include "Options.h"
#include "MonitorData.h"
#include "MonitorDataModel.h"
#include "TreeNode.h"

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
   model_( new MonitorDataModel())
{
  // Initialize the GUI and connect the signals and slots.
  this->ui.setupUi( this);
  connect( this->ui.repoAddButton,    SIGNAL(clicked()), this, SLOT(addRepo()));
  connect( this->ui.repoRemoveButton, SIGNAL(clicked()), this, SLOT(removeRepo()));
  connect( this->ui.quitButton,       SIGNAL(clicked()), this, SLOT(close()));

  connect( this->ui.repoSelection, SIGNAL(currentIndexChanged( const QString&)),
           this,                   SLOT(newRepo( const QString&)));

  connect( this->ui.repoView->header(), SIGNAL(sectionClicked(int)),
           this,                        SLOT(doSort(int)));

  // Initialize the model and tree view.
  this->dataSource_ = new MonitorData( this->options_, this->model_);
  this->ui.repoView->setModel( this->model_);

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

    // Setting the index results in the signal calling newRepo().
    int index = this->ui.repoSelection->findText( iorString);
    this->ui.repoSelection->setCurrentIndex( index);
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
Viewer::closeEvent( QCloseEvent* /* event */)
{
  this->ui.statusbar->showMessage( tr("Closing"));

  this->dataSource_->disable();
  delete this->model_;
  delete this->dataSource_;
}

} // End of namespace Monitor

