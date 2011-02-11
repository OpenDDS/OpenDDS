package org.opendds.modeling.sdk.model.GeneratorSpecification.Presentation;

import org.eclipse.jface.viewers.IContentProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.ViewerRow;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Item;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.Widget;

/**
 * @author martinezm
 * 
 * This class is intended to provide public access to protected abstract methods
 * from the StructuredViewer super class to allow an implementation of a
 * StructuredViewer to delegate the required operations to the contained
 * ListViewer instead of re-implementing those operations.
 *
 */
public class TableViewerDelegate extends TableViewer {

	public TableViewerDelegate(Composite parent) {
		super(parent);
	}

	public TableViewerDelegate(Table table) {
		super(table);
	}

	public TableViewerDelegate(Composite parent, int style) {
		super(parent, style);
	}

	protected void hookControl(Control control) {
		super.hookControl(control);
	}

	protected void handleDispose(DisposeEvent event) {
		super.handleDispose(event);
	}

	protected ViewerRow internalCreateNewRowPart(int style,
			int rowIndex) {
		return super.internalCreateNewRowPart(style, rowIndex);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.AbstractListViewer#setSelectionToWidget(java.util.List, boolean)
	 */
    @SuppressWarnings("unchecked")
	public void setSelectionToWidget(java.util.List in, boolean reveal) {
    	super.setSelectionToWidget( in, reveal);
	}
    

    /* (non-Javadoc)
     * Method declared on StructuredViewer.
     * Since SWT.List doesn't use items we always return the List itself.
     */
    protected Widget doFindInputItem(Object element) {
    	return super.doFindInputItem(element);
    }

    /* (non-Javadoc)
     * Method declared on StructuredViewer.
     * Since SWT.List doesn't use items we always return the List itself.
     */
    protected Widget doFindItem(Object element) {
    	return super.doFindItem(element);
    }

	/* (non-Javadoc)
     * Method declared on StructuredViewer.
     */
    protected void doUpdateItem(Widget data, Object element, boolean fullMap) {
    	super.doUpdateItem(data, element, fullMap);
    }

    protected Widget getColumnViewerOwner(int columnIndex) {
    	return super.getColumnViewerOwner(columnIndex);
	}

    /* (non-Javadoc)
     * Method declared on Viewer.
     */
    /* (non-Javadoc)
     * Method declared on StructuredViewer.
     */
    @SuppressWarnings("unchecked")
	protected java.util.List getSelectionFromWidget() {
    	return super.getSelectionFromWidget();
    }

    /**
     * @param element the element to insert
     * @return the index where the item should be inserted.
     */
    protected int indexForElement(Object element) {
    	return super.indexForElement(element);
    }

    /* (non-Javadoc)
     * Method declared on Viewer.
     */
    protected void inputChanged(Object input, Object oldInput) {
    	super.inputChanged(input, oldInput);
    }

    /* (non-Javadoc)
     * Method declared on StructuredViewer.
     */
    protected void internalRefresh(Object element) {
    	super.internalRefresh(element);
    }

    protected Object[] getRawChildren(Object parent) {
    	return super.getRawChildren(parent);
	}
	/*
	 * (non-Javadoc)
	 *
	 * @see org.eclipse.jface.viewers.StructuredViewer#assertContentProviderType(org.eclipse.jface.viewers.IContentProvider)
	 */
	protected void assertContentProviderType(IContentProvider provider) {
		super.assertContentProviderType(provider);
	}

	/**
	 * Searches the receiver's list starting at the first item (index 0) until
	 * an item is found that is equal to the argument, and returns the index of
	 * that item. If no item is found, returns -1.
	 *
	 * @param item
	 *            the search item
	 * @return the index of the item
	 *
	 * @since 3.3
	 */
	protected int doIndexOf(Item item) {
		return super.doIndexOf(item);
	}

	/**
	 * Returns the number of items contained in the receiver.
	 *
	 * @return the number of items
	 *
	 * @since 3.3
	 */
	protected int doGetItemCount() {
		return super.doGetItemCount();
	}

	/**
	 * Sets the number of items contained in the receiver.
	 *
	 * @param count
	 *            the number of items
	 *
	 * @since 3.3
	 */
	protected void doSetItemCount(int count) {
		super.doSetItemCount(count);
	}

	/**
	 * Returns a (possibly empty) array of TableItems which are the items in the
	 * receiver.
	 *
	 * @return the items in the receiver
	 *
	 * @since 3.3
	 */
	protected Item[] doGetItems() {
		return super.doGetItems();
	}

	/**
	 * Returns the column at the given, zero-relative index in the receiver.
	 * Throws an exception if the index is out of range. Columns are returned in
	 * the order that they were created. If no TableColumns were created by the
	 * programmer, this method will throw ERROR_INVALID_RANGE despite the fact
	 * that a single column of data may be visible in the table. This occurs
	 * when the programmer uses the table like a list, adding items but never
	 * creating a column.
	 *
	 * @param index
	 *            the index of the column to return
	 * @return the column at the given index
	 * @exception IllegalArgumentException -
	 *                if the index is not between 0 and the number of elements
	 *                in the list minus 1 (inclusive)
	 *
	 * @since 3.3
	 */
	protected Widget doGetColumn(int index) {
		return super.doGetColumn(index);
	}

	/**
	 * Returns the item at the given, zero-relative index in the receiver.
	 * Throws an exception if the index is out of range.
	 *
	 * @param index
	 *            the index of the item to return
	 * @return the item at the given index
	 * @exception IllegalArgumentException -
	 *                if the index is not between 0 and the number of elements
	 *                in the list minus 1 (inclusive)
	 *
	 * @since 3.3
	 */
	protected Item doGetItem(int index) {
		return super.doGetItem(index);
	}

	/**
	 * Returns an array of {@link Item} that are currently selected in the
	 * receiver. The order of the items is unspecified. An empty array indicates
	 * that no items are selected.
	 *
	 * @return an array representing the selection
	 *
	 * @since 3.3
	 */
	protected Item[] doGetSelection() {
		return super.doGetSelection();
	}

	/**
	 * Returns the zero-relative indices of the items which are currently
	 * selected in the receiver. The order of the indices is unspecified. The
	 * array is empty if no items are selected.
	 *
	 * @return an array representing the selection
	 *
	 * @since 3.3
	 */
	protected int[] doGetSelectionIndices() {
		return super.doGetSelectionIndices();
	}

	/**
	 * Clears all the items in the receiver. The text, icon and other attributes
	 * of the items are set to their default values. If the table was created
	 * with the <code>SWT.VIRTUAL</code> style, these attributes are requested
	 * again as needed.
	 *
	 * @since 3.3
	 */
	protected void doClearAll() {
		super.doClearAll();
	}

	/**
	 * Resets the given item in the receiver. The text, icon and other attributes
	 * of the item are set to their default values.
	 *
	 * @param item the item to reset
	 *
	 * @since 3.3
	 */
	protected void doResetItem(Item item) {
		super.doResetItem(item);
	}

	/**
	 * Removes the items from the receiver which are between the given
	 * zero-relative start and end indices (inclusive).
	 *
	 * @param start
	 *            the start of the range
	 * @param end
	 *            the end of the range
	 *
	 * @exception IllegalArgumentException -
	 *                if either the start or end are not between 0 and the
	 *                number of elements in the list minus 1 (inclusive)
	 *
	 * @since 3.3
	 */
	protected void doRemove(int start, int end) {
		super.doRemove(start, end);
	}

	/**
	 * Removes all of the items from the receiver.
	 *
	 * @since 3.3
	 */
	protected void doRemoveAll() {
		super.doRemoveAll();
	}

	/**
	 * Removes the items from the receiver's list at the given zero-relative
	 * indices.
	 *
	 * @param indices
	 *            the array of indices of the items
	 *
	 * @exception IllegalArgumentException -
	 *                if the array is null, or if any of the indices is not
	 *                between 0 and the number of elements in the list minus 1
	 *                (inclusive)
	 *
	 * @since 3.3
	 */
	protected void doRemove(int[] indices) {
		super.doRemove(indices);
	}

	/**
	 * Shows the item. If the item is already showing in the receiver, this
	 * method simply returns. Otherwise, the items are scrolled until the item
	 * is visible.
	 *
	 * @param item
	 *            the item to be shown
	 *
	 * @exception IllegalArgumentException -
	 *                if the item is null
	 *
	 * @since 3.3
	 */
	protected void doShowItem(Item item) {
		super.doShowItem(item);
	}

	/**
	 * Deselects all selected items in the receiver.
	 *
	 * @since 3.3
	 */
	protected void doDeselectAll() {
		super.doDeselectAll();
	}

	/**
	 * Sets the receiver's selection to be the given array of items. The current
	 * selection is cleared before the new items are selected.
	 * <p>
	 * Items that are not in the receiver are ignored. If the receiver is
	 * single-select and multiple items are specified, then all items are
	 * ignored.
	 * </p>
	 *
	 * @param items
	 *            the array of items
	 *
	 * @exception IllegalArgumentException -
	 *                if the array of items is null
	 *
	 * @since 3.3
	 */
	protected void doSetSelection(Item[] items) {
		super.doSetSelection(items);
	}

	/**
	 * Shows the selection. If the selection is already showing in the receiver,
	 * this method simply returns. Otherwise, the items are scrolled until the
	 * selection is visible.
	 *
	 * @since 3.3
	 */
	protected void doShowSelection() {
		super.doShowSelection();
	}

	/**
	 * Selects the items at the given zero-relative indices in the receiver. The
	 * current selection is cleared before the new items are selected.
	 * <p>
	 * Indices that are out of range and duplicate indices are ignored. If the
	 * receiver is single-select and multiple indices are specified, then all
	 * indices are ignored.
	 * </p>
	 *
	 * @param indices
	 *            the indices of the items to select
	 *
	 * @exception IllegalArgumentException -
	 *                if the array of indices is null
	 *
	 * @since 3.3
	 */
	protected void doSetSelection(int[] indices) {
		super.doSetSelection(indices);
	}

	/**
	 * Clears the item at the given zero-relative index in the receiver. The
	 * text, icon and other attributes of the item are set to the default value.
	 * If the table was created with the <code>SWT.VIRTUAL</code> style, these
	 * attributes are requested again as needed.
	 *
	 * @param index
	 *            the index of the item to clear
	 *
	 * @exception IllegalArgumentException -
	 *                if the index is not between 0 and the number of elements
	 *                in the list minus 1 (inclusive)
	 *
	 * @see SWT#VIRTUAL
	 * @see SWT#SetData
	 *
	 * @since 3.3
	 */
	protected void doClear(int index) {
		super.doClear(index);
	}



	/**
	 * Selects the items at the given zero-relative indices in the receiver.
	 * The current selection is not cleared before the new items are selected.
	 * <p>
	 * If the item at a given index is not selected, it is selected.
	 * If the item at a given index was already selected, it remains selected.
	 * Indices that are out of range and duplicate indices are ignored.
	 * If the receiver is single-select and multiple indices are specified,
	 * then all indices are ignored.
	 * </p>
	 *
	 * @param indices the array of indices for the items to select
	 *
	 * @exception IllegalArgumentException - if the array of indices is null
	 *
	 */
	protected void doSelect(int[] indices) {
		super.doSelect(indices);
	}

}
