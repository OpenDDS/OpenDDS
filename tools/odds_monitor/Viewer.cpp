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

namespace Monitor {

Viewer::Viewer( const Options& options, QMainWindow* parent)
 : QMainWindow( parent),
   options_( options),
   dataSource_( 0),
   model_( new MonitorDataModel())
{
  ui.setupUi( this);
  connect( ui.repoAddButton,    SIGNAL(clicked()), this, SLOT(addRepo()));
  connect( ui.repoRemoveButton, SIGNAL(clicked()), this, SLOT(removeRepo()));
  connect( ui.quitButton,       SIGNAL(clicked()), this, SLOT(close()));

  connect( ui.repoSelection, SIGNAL(currentIndexChanged( const QString&)),
           this,             SLOT(newRepo( const QString&)));

  this->dataSource_ = new MonitorData( this->options_, this->model_);
  this->ui.repoView->setModel( this->model_);
}

void
Viewer::newRepo( const QString& ior)
{
  if( !ior.isEmpty()) {
    if( this->dataSource_->setRepoIor( ior)) {
      ui.statusbar->showMessage( tr("Attached"));
      return;
    }
  }
  ui.statusbar->showMessage( tr("Detached"));
}

void
Viewer::addRepo()
{
  bool status = false;
  QString iorString = RepoSelect::getRepoIOR( this, &status);
  if( status && !iorString.isEmpty()) {
    ui.repoSelection->addItem( iorString);

    // Setting the index results in the signal calling newRepo().
    int index = ui.repoSelection->findText( iorString);
    ui.repoSelection->setCurrentIndex( index);
    // Status bar updated when the index change propagates.

  } else {
    ui.statusbar->showMessage( tr("Detached"));
  }
}

void
Viewer::removeRepo()
{
  this->dataSource_->removeRepo( ui.repoSelection->currentText());
  ui.repoSelection->removeItem( ui.repoSelection->currentIndex());

  // Status bar is updated when the index change propagates.
}

void
Viewer::closeEvent( QCloseEvent* /* event */)
{
  ui.statusbar->showMessage( tr("Closing"));

  this->dataSource_->disable();
  delete this->model_;
  delete this->dataSource_;
}

} // End of namespace Monitor

