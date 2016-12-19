/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef MONITORDATAMODEL_H
#define MONITORDATAMODEL_H

#include <QtCore/QAbstractItemModel>
#include <vector>

namespace Monitor {

class TreeNode;

/**
 * @class MonitorDataModel
 *
 * @brief Interface to a model encapsulating repository data.
 *
 * This class provides the interface for a view to interact with
 * encapsulated data of a repository model.
 */
class MonitorDataModel : public QAbstractItemModel {
  Q_OBJECT

  public:
    /**
     * @brief Construct with an optional parent link.
     *
     * @param parent link to parent object of this one
     */
    MonitorDataModel( QObject* parent = 0);

    virtual ~MonitorDataModel();

    /* Exposing data from the model */

    /// Obtain an index from a node.
    QModelIndex index( TreeNode* node, int column) const;

    /// Obtain a node from an index.
    TreeNode* getNode( const QModelIndex &index, bool defaultToRoot = true) const;

    /* Model update methods. */

    /// Change to a new tree of data.
    void newRoot( TreeNode* root);

    /// Data values have changed in a single node, update the view.
    void updated( TreeNode* node, int column);

    /// Data values have changed in a range of nodes, update the view.
    void updated( TreeNode* left,  int lcol, TreeNode* right, int rcol);

    /// Underlying model has changed, update the layout.
    void changed();

    /// Add a new node as a new row of data.
    void addData( int row, QList< QVariant> list, const QModelIndex& parent);

    //
    // QAbstractModel interfaces.
    //

    virtual QModelIndex index(
      int                row,
      int                column,
      const QModelIndex& parent = QModelIndex()
    ) const;

    virtual QModelIndex parent( const QModelIndex& index) const;

    /* Readonly interface methods */

    virtual QVariant data( const QModelIndex& index, int role) const;

    virtual Qt::ItemFlags flags( const QModelIndex& index) const;

    virtual QVariant headerData(
      int             section,
      Qt::Orientation orientation,
      int             role = Qt::DisplayRole
    ) const;

    virtual int rowCount( const QModelIndex& parent = QModelIndex()) const;

    virtual int columnCount( const QModelIndex& parent = QModelIndex()) const;

    /* Editable interface methods */

    virtual bool setData(
      const QModelIndex& index,
      const QVariant&    value,
      int                role = Qt::EditRole
    );

    virtual bool setHeaderData(
      int             section,
      Qt::Orientation orientation,
      const QVariant& value,
      int             role = Qt::EditRole
    );

    /* Resizable interface methods */

    virtual bool insertRows(
      int                row,
      int                count,
      const QModelIndex& parent = QModelIndex()
    );

    virtual bool removeRows(
      int                row,
      int                count,
      const QModelIndex& parent = QModelIndex()
    );

    virtual bool insertColumns(
      int                column,
      int                count,
      const QModelIndex& parent = QModelIndex()
    );

    virtual bool removeColumns(
      int                column,
      int                count,
      const QModelIndex& parent = QModelIndex()
    );

    /* Sorting */
    virtual void sort(
      int           column,
      Qt::SortOrder order = Qt::AscendingOrder
    );

    void doSort(
      int                column,
      Qt::SortOrder      order = Qt::AscendingOrder,
      const QModelIndex& index = QModelIndex()
    );

    /* Drag and drop support */
    /* Also requires insert rows/columns, remove rows/columns and setData */

    // Drag

    virtual QMimeData* mimeData( const QModelIndexList& indexes) const;

    // Drop

    virtual bool setItemData(
      const QModelIndex&         index,
      const QMap<int, QVariant>& roles
    );

    virtual Qt::DropActions supportedDropActions() const;

    virtual QStringList mimeTypes() const;

    virtual bool dropMimeData(
      const QMimeData*   data,
      Qt::DropAction     action,
      int                row,
      int                column,
      const QModelIndex& parent
    );

    virtual bool removeRow(
      int                row,
      const QModelIndex& parent = QModelIndex()
    );

    virtual bool removeColumn(
      int                column,
      const QModelIndex& parent = QModelIndex()
    );

    /* Optimization methods */

    virtual bool hasChildren(
      const QModelIndex& parent = QModelIndex()
    ) const;

    virtual bool canFetchMore( const QModelIndex& parent) const;

    virtual void fetchMore( const QModelIndex& parent);

  private:
    /// Root node of the tree.
    TreeNode* root_;
};

} // End of namespace Monitor

#endif /* MONITORDATAMODEL_H */

