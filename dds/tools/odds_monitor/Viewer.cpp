/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <Qt/QtGui>
#include "Viewer.h"
#include "MonitorDataModel.h"
#include "TreeNode.h"

namespace Monitor {

Viewer::Viewer( QMainWindow* parent)
 : QMainWindow( parent),
   model_( 0),
   root_( 0)
{
  ui.setupUi( this);
  connect( ui.action_String,   SIGNAL(triggered()), this, SLOT(getIor()));
  connect( ui.actionFile,      SIGNAL(triggered()), this, SLOT(openFile()));
  connect( ui.actionQuit,      SIGNAL(triggered()), this, SLOT(close()));
  connect( ui.quitButton,      SIGNAL(clicked()),   this, SLOT(close()));
  connect( ui.changeNowButton, SIGNAL(clicked()),   this, SLOT(modifyTree()));

  QList< QVariant> list;
  list << QString( "Element")
       << QString( "Value");

  this->root_ = new TreeNode( list);
  this->model_ = new MonitorDataModel( this->root_);
  this->ui.repoView->setModel( this->model_);
}

void
Viewer::openFile()
{
  QString fileName = QFileDialog::getOpenFileName( this);
  if( !fileName.isEmpty()) {
    QList< QVariant> list;
    list << QString( "File") << fileName;
    this->model_->addData( 0, list, this->model_->index( 0, 0).parent());
  }
}

void
Viewer::getIor()
{
  bool status = false;
  QString iorString = QInputDialog::getText(
                        this,
			tr("Stringified Inter ORB Reference"),
			tr("Input an IOR"),
			QLineEdit::Normal,
			tr("corbaloc::iiop:"),
			&status
                      );
  if( status && !iorString.isEmpty()) {
    QList< QVariant> list;
    list << QString( "IOR") << iorString;
    this->model_->addData( 0, list, this->ui.repoView->currentIndex());
  }
}

void
Viewer::modifyTree()
{
  static int counter = 0;

  QString left("NewNode ");
  left.append( QVariant( ++counter).toString());
  QString right( "NewValue ");
  right.append( QVariant( ++counter).toString());

  QList< QVariant> list;
  list << left << right;

  this->model_->addData(
    1 + this->ui.repoView->currentIndex().row(),
    list,
    this->ui.repoView->currentIndex().parent()
  );
}

void
Viewer::closeEvent( QCloseEvent* /* event */)
{
}

} // End of namespace Monitor

