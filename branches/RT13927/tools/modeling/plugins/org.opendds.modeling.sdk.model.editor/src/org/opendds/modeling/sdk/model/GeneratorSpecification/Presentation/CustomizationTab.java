/**
 * 
 */
package org.opendds.modeling.sdk.model.GeneratorSpecification.Presentation;

import java.util.List;

import org.eclipse.jface.viewers.StructuredViewer;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Tree;
import org.eclipse.swt.widgets.Widget;

/**
 * @author martinezm
 *
 */
public class CustomizationTab extends StructuredViewer {
	// A TreeViewer with the pretected abstract StructuredViewer methods
	// exposed so that we can simply delegate to them.
	TreeViewerDelegate treeViewer;

	/**
	 * 
	 */
	public CustomizationTab( Composite parent) {
		treeViewer = new TreeViewerDelegate( parent);
		
		// TODO Add the stuff to surround the actual tree viewer.
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.StructuredViewer#doFindInputItem(java.lang.Object)
	 */
	@Override
	protected Widget doFindInputItem(Object element) {
		return treeViewer.doFindInputItem(element);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.StructuredViewer#doFindItem(java.lang.Object)
	 */
	@Override
	protected Widget doFindItem(Object element) {
		return treeViewer.doFindItem(element);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.StructuredViewer#doUpdateItem(org.eclipse.swt.widgets.Widget, java.lang.Object, boolean)
	 */
	@Override
	protected void doUpdateItem(Widget item, Object element, boolean fullMap) {
		treeViewer.doUpdateItem(item, element, fullMap);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.StructuredViewer#getSelectionFromWidget()
	 */
	@Override
	protected List<?> getSelectionFromWidget() {
		return treeViewer.getSelectionFromWidget();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.StructuredViewer#internalRefresh(java.lang.Object)
	 */
	@Override
	protected void internalRefresh(Object element) {
		treeViewer.internalRefresh(element);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.StructuredViewer#reveal(java.lang.Object)
	 */
	@Override
	public void reveal(Object element) {
		// TODO Auto-generated method stub / with delegation.
		treeViewer.reveal( element);

	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.StructuredViewer#setSelectionToWidget(java.util.List, boolean)
	 * 
	 * List<Object>
	 */
	@SuppressWarnings("unchecked")
	@Override
	protected void setSelectionToWidget(List l, boolean reveal) {
		treeViewer.setSelectionToWidget(l, reveal);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.Viewer#getControl()
	 */
	@Override
	public Control getControl() {
		// TODO Auto-generated method stub / with delegation.
		return treeViewer.getControl();
	}

	// From TreeViewer
	public Tree getTree() {
		return treeViewer.getTree();
	}

}
