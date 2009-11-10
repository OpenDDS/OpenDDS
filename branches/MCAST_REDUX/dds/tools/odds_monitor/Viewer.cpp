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

#include <iostream>

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
}

void
Viewer::openFile()
{
  QString fileName = QFileDialog::getOpenFileName( this);
  if( !fileName.isEmpty()) {
    std::cout << "FILENAME: " << fileName.toStdString() << std::endl;
    delete this->model_;
    this->model_ = this->loadStubData();
    this->ui.repoView->setModel( this->model_);
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
    std::cout << "IOR: " << iorString.toStdString() << std::endl;
    delete this->model_;
    this->model_ = this->loadStubData();
    this->ui.repoView->setModel( this->model_);

  } else {
    std::cout << "NO JOY!" << std::endl;
  }
}

void
Viewer::modifyTree()
{
std::cout << "NO ";
  static int counter = 0;
  QList< QVariant> list;
  list << QString( "Root") << QString( "RootValue");
  this->root_ = new TreeNode( list);

  QString left("NewNode ");
  left.append( QVariant( ++counter).toString());
  QString right( "NewValue ");
  right.append( QVariant( ++counter).toString());

  list.clear();
  list << left << right;
  this->root_->append( new TreeNode( list, this->root_));

  delete this->model_;
  this->model_ = new MonitorDataModel( this->root_);
  this->ui.repoView->setModel( this->model_);

  std::cout << "JOY!" << std::endl;
  return;
//  this->model_->insertRows( 1, 1);
//  QModelIndex newIndex = this->model_index( 0, 0);
  this->root_->append( new TreeNode( list, this->root_));
//  this->model_->dataChanged( QModelIndex(), QModelIndex());
  this->model_->addData( 0, list);
}

void
Viewer::closeEvent( QCloseEvent* /* event */)
{
  std::cerr << "CLOSING!" << std::endl;
}

MonitorDataModel*
Viewer::loadStubData()
{
  static int entry = 0;
  static int counter = 0;

  this->root_ = 0;
  switch( ++entry) {
    case 1:
      {
        QList< QVariant> list;
        list << QString( "Root") << QString( "RootValue");
        this->root_ = new TreeNode( list);

        list.clear();
        list << QString( "Node1") << QString( "Value1");
        TreeNode* node1 = new TreeNode( list, this->root_);
        this->root_->append( node1);

        list.clear();
        list << QString( "Node2") << QString( "Value2");
        this->root_->append( new TreeNode( list, this->root_));

        list.clear();
        list << QString( "Node3") << QString( "Value3");
        node1->append( new TreeNode( list, node1));
      }
      break;

    default:
      {
        QList< QVariant> list;
        list << QString( "Root") << QString( "RootValue");
        this->root_ = new TreeNode( list);

        list.clear();
        list << QString( "Empty") << QString( "nothing");
        this->root_->append( new TreeNode( list, this->root_));

        list.clear();
        list << QString( "Stuff") << QString( "something");
        TreeNode* node2 = new TreeNode( list, this->root_);
        this->root_->append( node2);

        for( int i = 0; i < entry; ++i) {
          list.clear();
          QString left("Node");
          left.append( QVariant( ++counter).toString());
          QString right( "Value");
          right.append( QVariant( ++counter).toString());
          list << left << right;
          node2->append( new TreeNode( list, node2));
        }
      }
      break;
  }

  return new MonitorDataModel( this->root_);
}

} // End of namespace Monitor

