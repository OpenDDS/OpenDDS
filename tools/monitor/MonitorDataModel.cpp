/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MonitorDataModel.h"
#include "TreeNode.h"

#include <QtCore/QStringList>
#include <QtGui/QTreeView>

namespace Monitor {

MonitorDataModel::MonitorDataModel( QObject* parent)
 : QAbstractItemModel( parent)
{
  // Tree view header values are invariant for this model.
  QList< QVariant> list;
  list << QString( "Element")
       << QString( "Value");
  this->root_ = new TreeNode( list);
}

MonitorDataModel::~MonitorDataModel()
{
  // Cascade delete through the tree data.
  delete this->root_;
}

void
MonitorDataModel::newRoot( TreeNode* root)
{
  delete this->root_;
  this->root_ = root;
  this->reset();
}

void
MonitorDataModel::updated(
  TreeNode* left,  int lcol,
  TreeNode* right, int rcol
)
{
  QModelIndex topLeft     = this->index( left,  lcol);
  QModelIndex bottomRight = this->index( right, rcol);

  emit dataChanged( topLeft, bottomRight);
}

void
MonitorDataModel::updated( TreeNode* node, int column)
{
  QModelIndex index = this->index( node, column);
  emit dataChanged( index, index);
}

void
MonitorDataModel::changed()
{
  emit layoutChanged();
}

QModelIndex
MonitorDataModel::index( TreeNode* node, int column) const
{
  // Treat null nodes as the root.
  if( !node) node = this->root_;

  // We have to reach the root before we can start forming indices.
  // Retain the row information for the entire path as we traverse it.
  QList<int> list;
  for( ; node->parent(); node = node->parent()) {
    list.append( node->row());
  }

  // Form the index of the tree root to start from.  Use the topmost
  // non-null node that we found as the root.
  QModelIndex index = this->QAbstractItemModel::createIndex( 0, column, node);

  // Now we can form index values all the way back to the node of
  // interest.
  while( !list.isEmpty()) {
    index = this->index( list.takeLast(), column, index);
  }
  return index;
}

TreeNode*
MonitorDataModel::getNode( const QModelIndex &index, bool defaultToRoot) const
{
  if( index.isValid()) {
    TreeNode* node = static_cast< TreeNode*>( index.internalPointer());
    if( node) {
      return node;
    }
  }

  if (defaultToRoot) {
    return this->root_;
  } else {
    return NULL;
  }
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

  TreeNode* parentNode = this->getNode( parent);
  TreeNode* childNode  = (*parentNode)[ row];
  if( childNode) {
    return this->QAbstractItemModel::createIndex( row, column, childNode);

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

  TreeNode* childNode  = this->getNode( index);
  TreeNode* parentNode = childNode->parent();

  if( parentNode == this->root_ || parentNode == 0) {
    return QModelIndex();
  }

  return this->QAbstractItemModel::createIndex( parentNode->row(), 0, parentNode);
}

QVariant
MonitorDataModel::data( const QModelIndex& index, int role) const
{
  if( false == index.isValid()) {
    return QVariant();
  }

  switch( role) {
    case Qt::DisplayRole:
      return this->getNode( index)->column( index.column());

    // for checkboxes in tree view
    // case Qt::CheckStateRole:
    //   return Qt::Checked;
    case Qt::ToolTipRole:
    case Qt::StatusTipRole:
    case Qt::WhatsThisRole:
    default:
      return QVariant();

    case Qt::BackgroundRole:
      {
        QVariant value = this->getNode( index)->color( index.column());
        if( value.value<QColor>().isValid()) {
          return value;
        } else {
          return QVariant();
        }
      }
  }
}

Qt::ItemFlags
MonitorDataModel::flags( const QModelIndex& index) const
{
  if( false == index.isValid()) {
    return 0;
  }

  return this->QAbstractItemModel::flags( index)
       | Qt::ItemIsEnabled
       | Qt::ItemIsSelectable
  //     | Qt::ItemIsUserCheckable  // checkboxes in tree view
  //     | Qt::ItemIsEditable
  //     | Qt::ItemIsDragEnabled
  //     | Qt::ItemIsDropEnabled
  ;
}

QVariant
MonitorDataModel::headerData(
  int             section,
  Qt::Orientation /* orientation */,
  int             role
) const
{
  if( role == Qt::DisplayRole) {
    return this->root_->column( section);
  }

  return QVariant();
}

int
MonitorDataModel::rowCount( const QModelIndex& parent) const
{
  if( parent.column() > 0) {
    return 0;
  }

  return this->getNode( parent)->size();
}

int
MonitorDataModel::columnCount( const QModelIndex& parent) const
{
  return this->getNode( parent)->width();
}

bool
MonitorDataModel::setData(
  const QModelIndex& index,
  const QVariant&    value,
  int                role
)
{
  switch( role) {
//  case Qt::EditRole:
//    if( index.column() == 0) return false;
//    break;

    case Qt::DisplayRole:
      this->getNode( index)->setData( index.column(), value);
      break;

    case Qt::BackgroundRole:
      // QColor(255,191,191);
      this->getNode( index)->setColor( index.column(), value);
      break;

    default: return false;
  }

  emit dataChanged( index, index);
  return true;
}

bool
MonitorDataModel::setHeaderData(
  int             section,
  Qt::Orientation /* orientation */,
  const QVariant& value,
  int             role
)
{
  switch( role) {
    case Qt::DisplayRole:
      this->root_->setData( section, value);
      // this->getNode( index)->setColor( section, QColor(255,255,255));
      break;

    case Qt::BackgroundRole:
      this->root_->setColor( section, value);
      break;

    default: return false;
  }

  emit
  dataChanged( QModelIndex(), QModelIndex());
  return true;
}

bool
MonitorDataModel::insertRows(
  int                row,
  int                count,
  const QModelIndex& parent
)
{
  bool success;

  beginInsertRows( parent, row, row+count-1);
  success = this->getNode( parent)->insertChildren(
              row,
              count,
              this->root_->width()
            );
  endInsertRows();

  emit layoutChanged();

  return success;
}

bool
MonitorDataModel::removeRows(
  int                row,
  int                count,
  const QModelIndex& parent
)
{
  bool success;

  beginRemoveRows( parent, row, row+count-1);
  success = this->getNode( parent)->removeChildren( row, count);
  endRemoveRows();
  emit layoutChanged();

  return success;
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

void
MonitorDataModel::sort(
  int           column,
  Qt::SortOrder order
)
{
  this->doSort( column, order);
}

void
MonitorDataModel::doSort(
  int                column,
  Qt::SortOrder      order,
  const QModelIndex& index
)
{
  TreeNode* node = this->getNode( index);
  if( !node) {
    node = this->root_;
  }
  node->sort( column, order);
  emit layoutChanged();
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
  return this->getNode( parent)->size() > 0;
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
MonitorDataModel::addData( int row, QList< QVariant> list, const QModelIndex& parent)
{
  this->insertRows( row, 1, parent);
  int column = 0;
  Q_FOREACH( QVariant current, list) {
    QModelIndex index = this->index( row, column, parent);
    this->setData( index, list.at( column++), Qt::DisplayRole);
  }
}

} // End of namespace Monitor

