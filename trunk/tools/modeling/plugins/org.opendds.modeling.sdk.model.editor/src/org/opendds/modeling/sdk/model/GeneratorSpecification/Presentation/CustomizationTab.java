/**
 * 
 */
package org.opendds.modeling.sdk.model.GeneratorSpecification.Presentation;

import java.util.List;

import org.eclipse.jface.viewers.IBaseLabelProvider;
import org.eclipse.jface.viewers.IContentProvider;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.StructuredViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.dnd.DragSourceListener;
import org.eclipse.swt.dnd.DropTargetListener;
import org.eclipse.swt.dnd.Transfer;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Tree;
import org.eclipse.swt.widgets.Widget;

/**
 * @author martinezm
 *
 */
public class CustomizationTab extends StructuredViewer {
	// A TreeViewer with the protected abstract StructuredViewer methods
	// exposed so that we can simply delegate to them.
	protected TreeViewerDelegate treeViewer;
	
	protected Composite control;

	/**
	 * 
	 */
	public CustomizationTab( Composite parent) {
		control = new Composite(parent, 0);
		
		control.setLayout( new GridLayout( 2, true));
		{
			Composite panel = new Composite(control, 0);
			GridData panelData = new GridData(SWT.FILL, SWT.FILL, true, true);
			panel.setLayoutData(panelData);
			
			GridLayout layout = new GridLayout(1, true);
			panel.setLayout(layout);
			{
				Label label = new Label(panel, 0);
				label.setText("Model Instance Definitions");
				GridData data = new GridData(SWT.CENTER, SWT.TOP, false, false);
				label.setLayoutData(data);
			}
			
			{
				Composite treePane = new Composite(panel,0);
				GridData data = new GridData(SWT.FILL, SWT.FILL, true, true);
				treePane.setLayoutData(data);
				
				treePane.setLayout( new FillLayout(SWT.VERTICAL));
				treeViewer = new TreeViewerDelegate( treePane, SWT.FULL_SELECTION);
			}
			
		}
		
		{
			Composite panel = new Composite(control, 0);
			GridData panelData = new GridData(SWT.LEAD, SWT.FILL, true, true);
			panel.setLayoutData(panelData);
			
			GridLayout layout = new GridLayout(1, true);
			panel.setLayout(layout);
			{
				Label label = new Label(panel, 0);
				label.setText("Selection Information");
				GridData data = new GridData(SWT.CENTER, SWT.TOP, true, true);
				label.setLayoutData(data);
				
				// TODO add content here
			}
		}
		
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.StructuredViewer#addDragSupport(int, org.eclipse.swt.dnd.Transfer[], org.eclipse.swt.dnd.DragSourceListener)
	 */
	@Override
	public void addDragSupport(int operations, Transfer[] transferTypes, DragSourceListener listener) {
		treeViewer.addDragSupport(operations, transferTypes, listener);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.StructuredViewer#addDragSupport(int, org.eclipse.swt.dnd.Transfer[], org.eclipse.swt.dnd.DropTargetListener)
	 */
	@Override
	public void addDropSupport(int operations, Transfer[] transferTypes, final DropTargetListener listener) {
		treeViewer.addDropSupport(operations, transferTypes, listener);
	}

	/*
	 * @see org.eclipse.jface.viewers.Viewer#addSelectionChangedListener()
	 */
	@Override
	public void addSelectionChangedListener(ISelectionChangedListener listener) {
		treeViewer.addSelectionChangedListener(listener);
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

	/*
	 * @see org.eclipse.jface.viewers.StructuredViewer#getSelection()
	 */
	@Override
	public ISelection getSelection() {
		return treeViewer.getSelection();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.StructuredViewer#getSelectionFromWidget()
	 */
	@Override
	protected List<?> getSelectionFromWidget() {
		return treeViewer.getSelectionFromWidget();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.AbstractTreeViewer#inputChanged()
	 */
	@Override
	protected void inputChanged(Object input, Object oldInput) {
		treeViewer.inputChanged(input, oldInput);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.StructuredViewer#internalRefresh(java.lang.Object)
	 */
	@Override
	protected void internalRefresh(Object element) {
		treeViewer.internalRefresh(element);
	}
	
	/*
	 * @see org.eclipse.jface.viewers.Viewer#removeSelectionChangedListener()
	 */
	@Override
    public void removeSelectionChangedListener( ISelectionChangedListener listener) {
    	treeViewer.removeSelectionChangedListener(listener);
    }

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.StructuredViewer#reveal(java.lang.Object)
	 */
	@Override
	public void reveal(Object element) {
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
		return control;
	}
	
	public Control getTreeControl() {
		return treeViewer.getControl();
	}

	// From TreeViewer
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.AbstractTreeViewer#setContentProvider(org.eclipse.jface.viewers.IContentProvider)
	 */
	@Override
	public void setContentProvider(IContentProvider provider) {
		treeViewer.setContentProvider(provider);
	}

	// Cannot override the final StructuredViewer implementation.  This
	// may cause some unforseen issues if code using the StructuredViewer
	// interfaces uses this tab form instead.
	public void setTreeInput(Object input) {
		treeViewer.setInput(input);
	}

	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ColumnViewer#setLabelProvider(org.eclipse.jface.viewers.IBaseLabelProvider)
	 */
	@Override
	public void setLabelProvider(IBaseLabelProvider labelProvider) {
		treeViewer.setLabelProvider(labelProvider);
	}

	/*
	 * @see org.eclipse.jface.viewers.Viewer#setSelection()
	 */
	@Override
	public void setSelection( ISelection selection) {
		treeViewer.setSelection( selection);
	}
	
	public Tree getTree() {
		return treeViewer.getTree();
	}

}
