/**
 * 
 */
package org.opendds.modeling.sdk.model.GeneratorSpecification.Presentation;

import java.util.List;

import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Tree;
import org.eclipse.swt.widgets.Widget;

/**
 * @author martinezm
 * 
 * This class is intended to provide public access to protected abstract methods
 * from the StructuredViewer super class to allow an implementation of a
 * StructuredViewer to delegate the required operations to the contained
 * TreeViewer instead of re-implementing those operations.
 *
 */
public class TreeViewerDelegate extends TreeViewer {

	/**
	 * @param parent
	 */
	public TreeViewerDelegate(Composite parent) {
		super(parent);
	}

	/**
	 * @param tree
	 */
	public TreeViewerDelegate(Tree tree) {
		super(tree);
	}

	/**
	 * @param parent
	 * @param style
	 */
	public TreeViewerDelegate(Composite parent, int style) {
		super(parent, style);
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.StructuredViewer#doFindInputItem(java.lang.Object)
	 */
	@Override
	public Widget doFindInputItem(Object element) {
		return super.doFindInputItem(element);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.StructuredViewer#doFindItem(java.lang.Object)
	 */
	@Override
	public Widget doFindItem(Object element) {
		return super.doFindItem(element);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.StructuredViewer#doUpdateItem(org.eclipse.swt.widgets.Widget, java.lang.Object, boolean)
	 */
	@Override
	public void doUpdateItem(Widget item, Object element, boolean fullMap) {
		super.doUpdateItem(item, element, fullMap);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.StructuredViewer#getSelectionFromWidget()
	 */
	@Override
	public List<?> getSelectionFromWidget() {
		return super.getSelectionFromWidget();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.AbstractTreeViewer#inputChanged()
	 */
	@Override
	public void inputChanged(Object input, Object oldInput) {
		super.inputChanged(input, oldInput);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.StructuredViewer#internalRefresh(java.lang.Object)
	 */
	@Override
	public void internalRefresh(Object element) {
		super.internalRefresh(element);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.StructuredViewer#setSelectionToWidget(java.util.List, boolean)
	 * 
	 * List<Object>
	 */
	@SuppressWarnings("unchecked")
	@Override
	public void setSelectionToWidget(List l, boolean reveal) {
		super.setSelectionToWidget(l, reveal);
	}

	@Override
	public void addSelectionChangedListener( ISelectionChangedListener listener) {
		super.addSelectionChangedListener(listener);
	}
	
	@Override
	public void removeSelectionChangedListener( ISelectionChangedListener listener) {
		super.removeSelectionChangedListener(listener);
	}
}
