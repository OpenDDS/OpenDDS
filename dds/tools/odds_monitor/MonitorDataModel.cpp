/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MonitorDataModel.h"
#include "TreeNode.h"

#include <QtCore/QStringList>

namespace Monitor {

MonitorDataModel::MonitorDataModel( TreeNode* root, QObject* parent)
 : QAbstractItemModel( parent),
   root_( root)
{
}

MonitorDataModel::~MonitorDataModel()
{
  // Cascade delete through the tree data.
  delete this->root_;
}

QModelIndex
MonitorDataModel::index(
  int                row,
  int                column,
  const QModelIndex& parent
) const
{
  if( false == this->hasIndex( row, column, parent)) {
    return QModelIndex();
  }

  TreeNode* parentNode = this->root_;
  if( parent.isValid()) {
    parentNode = static_cast<TreeNode*>( parent.internalPointer());
  }

  TreeNode* childNode = (*parentNode)[ row];
  if( childNode) {
    return createIndex( row, column, childNode);

  } else {
    return QModelIndex();
  }
}

QModelIndex
MonitorDataModel::parent( const QModelIndex& index) const
{
  if( false == index.isValid()) {
    return QModelIndex();
  }

  TreeNode* childNode  = static_cast<TreeNode*>( index.internalPointer());
  TreeNode* parentNode = childNode->parent();

  if( parentNode == this->root_) {
    return QModelIndex();
  }

  return createIndex( parentNode->row(), 0, parentNode);
}

QVariant
MonitorDataModel::data( const QModelIndex& index, int role) const
{
  if( false == index.isValid()) {
    return QVariant();
  }

  switch( role) {
    case Qt::DisplayRole:
      return static_cast<TreeNode*>( index.internalPointer())->column( index.column());

    case Qt::ToolTipRole:
    case Qt::StatusTipRole:
    case Qt::WhatsThisRole:
    default:
      return QVariant();
  }
}

Qt::ItemFlags
MonitorDataModel::flags( const QModelIndex& index) const
{
  if( false == index.isValid()) {
    return Qt::ItemIsEnabled;
  }

  return this->QAbstractItemModel::flags( index)
         | Qt::ItemIsEditable
  //     | Qt::ItemIsDragEnabled
  //     | Qt::ItemIsDropEnabled
         ;
}

QVariant
MonitorDataModel::headerData(
  int             section,
  Qt::Orientation orientation,
  int             role
) const
{
  return this->QAbstractItemModel::headerData( section, orientation, role);
}

int
MonitorDataModel::rowCount( const QModelIndex& parent) const
{
  if( parent.column() > 0) {
    return 0;
  }

  TreeNode* parentNode = this->root_;
  if( parent.isValid()) {
    parentNode = static_cast<TreeNode*>( parent.internalPointer());
  }

  return parentNode->size();
}

int
MonitorDataModel::columnCount( const QModelIndex& parent) const
{
  if( parent.isValid()) {
    return static_cast<TreeNode*>( parent.internalPointer())->width();

  } else {
    return this->root_->width();
  }
}

bool
MonitorDataModel::setData(
  const QModelIndex& index,
  const QVariant&    value,
  int                role
)
{
  return this->QAbstractItemModel::setData( index, value, role);
}

bool
MonitorDataModel::setHeaderData(
  int             section,
  Qt::Orientation orientation,
  const QVariant& value,
  int             role
)
{
  return this->QAbstractItemModel::setHeaderData( section, orientation, value, role);
}

bool
MonitorDataModel::insertRows(
  int                row,
  int                count,
  const QModelIndex& parent
)
{
  return this->QAbstractItemModel::insertRows( row, count, parent);
}

bool
MonitorDataModel::removeRows(
  int                row,
  int                count,
  const QModelIndex& parent
)
{
  return this->QAbstractItemModel::removeRows( row, count, parent);
}

bool
MonitorDataModel::insertColumns(
  int                column,
  int                count,
  const QModelIndex& parent
)
{
  return this->QAbstractItemModel::insertColumns( column, count, parent);
}

bool
MonitorDataModel::removeColumns(
  int                column,
  int                count,
  const QModelIndex& parent
)
{
  return this->QAbstractItemModel::removeColumns( column, count, parent);
}

QMimeData*
MonitorDataModel::mimeData( const QModelIndexList& indexes) const
{
  return this->QAbstractItemModel::mimeData( indexes);
}

bool
MonitorDataModel::setItemData(
  const QModelIndex&         index,
  const QMap<int, QVariant>& roles
)
{
  return this->QAbstractItemModel::setItemData( index, roles);
}

Qt::DropActions
MonitorDataModel::supportedDropActions() const
{
  return this->QAbstractItemModel::supportedDropActions();
}

QStringList
MonitorDataModel::mimeTypes() const
{
  return this->QAbstractItemModel::mimeTypes();
}

bool
MonitorDataModel::dropMimeData(
  const QMimeData*   data,
  Qt::DropAction     action,
  int                row,
  int                column,
  const QModelIndex& parent
)
{
  return this->QAbstractItemModel::dropMimeData(
           data,
           action,
           row,
           column,
           parent
         );
}

bool
MonitorDataModel::removeRow(
  int                row,
  const QModelIndex& parent
)
{
  return this->QAbstractItemModel::removeRow( row, parent);
}

bool
MonitorDataModel::removeColumn(
  int                column,
  const QModelIndex& parent
)
{
  return this->QAbstractItemModel::removeColumn( column, parent);
}

bool
MonitorDataModel::hasChildren(
  const QModelIndex& parent
) const
{
  return this->QAbstractItemModel::hasChildren( parent);
}

bool
MonitorDataModel::canFetchMore( const QModelIndex& parent) const
{
  return this->QAbstractItemModel::canFetchMore( parent);
}

void
MonitorDataModel::fetchMore( const QModelIndex& parent)
{
  this->QAbstractItemModel::fetchMore( parent);
}

void
MonitorDataModel::load( const QStringList& /* lines */, TreeNode* /* parent */)
{
}

void
MonitorDataModel::addData( int row, QList< QVariant> list)
{
  this->layoutChanged();
}

} // End of namespace Monitor

