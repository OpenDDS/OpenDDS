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
#include "Options.h"
#include "MonitorData.h"
#include "MonitorDataModel.h"
#include "TreeNode.h"

namespace Monitor {

Viewer::Viewer( const Options& options, QMainWindow* parent)
 : QMainWindow( parent),
   options_( options),
   dataSource_( 0),
   model_( 0),
   root_( 0)
{
  ui.setupUi( this);
  connect( ui.actionIOR_String, SIGNAL(triggered()), this, SLOT(getIor()));
  connect( ui.actionIOR_File,   SIGNAL(triggered()), this, SLOT(openFile()));
  connect( ui.actionQuit,       SIGNAL(triggered()), this, SLOT(close()));
  connect( ui.quitButton,       SIGNAL(clicked()),   this, SLOT(close()));

  // Tree view header values.
  QList< QVariant> list;
  list << QString( "Element")
       << QString( "Value");

  this->root_       = new TreeNode( list);
  this->model_      = new MonitorDataModel( this->root_);
  this->dataSource_ = new MonitorData( this->options_, this->model_);
  this->ui.repoView->setModel( this->model_);
}

void
Viewer::stubmodelchange()
{
  // Tree view header values.
  QList< QVariant> list;
  list << QString( "Element")
       << QString( "Value");
  this->root_ = new TreeNode( list);

  static int which = 0;
  TreeNode* parent = this->root_;
  if( ++which%2) {
    for( int row = 0; row < 4; ++row) {
      std::stringstream buffer1;
      buffer1 << "SomeProperty at " << row << std::ends;
      QString element(buffer1.str().c_str());

      std::stringstream buffer2;
      buffer2 << "some value with which = " << which << " at " << row << std::ends;
      QString value(buffer2.str().c_str());

      QList<QVariant> data;
      data << element << value;

      TreeNode* node = new TreeNode( data, parent);
      parent->append( node);
    }

  } else {
    for( int row = 0; row < 4; ++row) {
      std::stringstream buffer1;
      buffer1 << "AnotherProperty at " << row << std::ends;
      QString element(buffer1.str().c_str());

      std::stringstream buffer2;
      buffer2 << "another value with which = " << which << " at " << row << std::ends;
      QString value(buffer2.str().c_str());

      QList<QVariant> data;
      data << element << value;

      TreeNode* node = new TreeNode( data, parent);
      parent->append( node);
    }
  }

  this->model_->newRoot( this->root_);
}

void
Viewer::openFile()
{
  QString fileName = QFileDialog::getOpenFileName( this);
  if( !fileName.isEmpty()) {
    QString fileIor( "file://");
    fileIor.append( fileName);
    this->dataSource_->setRepoIor( fileIor);
    ui.statusbar->showMessage(fileIor);

    stubmodelchange();

    /// Stub a visible result.
    //QList< QVariant> list;
    //list << QString( "File") << fileName;
    //this->model_->addData( 0, list, this->model_->index( 0, 0).parent());
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
    this->dataSource_->setRepoIor( iorString);
    ui.statusbar->showMessage(iorString);

    stubmodelchange();

    /// Stub a visible result.
    //QList< QVariant> list;
    //list << QString( "IOR") << iorString;
    //this->model_->addData( 0, list, this->ui.repoView->currentIndex());
  }
}

void
Viewer::closeEvent( QCloseEvent* /* event */)
{
  this->dataSource_->disable();
  delete this->model_;
  delete this->dataSource_;
}

} // End of namespace Monitor

