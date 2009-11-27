/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <Qt/QtGui>
#include <sstream>
#include "Viewer.h"
#include "RepoSelect.h"
#include "Options.h"
#include "MonitorData.h"
#include "MonitorDataModel.h"
#include "TreeNode.h"

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
  this->ui.setupUi( this);
  connect( this->ui.repoAddButton,    SIGNAL(clicked()), this, SLOT(addRepo()));
  connect( this->ui.repoRemoveButton, SIGNAL(clicked()), this, SLOT(removeRepo()));
  connect( this->ui.quitButton,       SIGNAL(clicked()), this, SLOT(close()));

  connect( this->ui.repoSelection, SIGNAL(currentIndexChanged( const QString&)),
           this,                   SLOT(newRepo( const QString&)));

  this->dataSource_ = new MonitorData( this->options_, this->model_);
  this->ui.repoView->setModel( this->model_);

  this->ui.repoSelection->addItem( repoDetachedSelection);
}

void
Viewer::newRepo( const QString& ior)
{
  if( ior != repoDetachedSelection) {
    if( !ior.isEmpty()) {
      if( this->dataSource_->setRepoIor( ior)) {
        this->ui.statusbar->showMessage( tr("Attached"));

      } else {
        this->ui.statusbar->showMessage( tr("Failed to attach"));
      }
    }
  } else {
    this->dataSource_->clearData();
    this->ui.statusbar->showMessage( tr("Detached"));
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
Viewer::closeEvent( QCloseEvent* /* event */)
{
  this->ui.statusbar->showMessage( tr("Closing"));

  this->dataSource_->disable();
  delete this->model_;
  delete this->dataSource_;
}

} // End of namespace Monitor

