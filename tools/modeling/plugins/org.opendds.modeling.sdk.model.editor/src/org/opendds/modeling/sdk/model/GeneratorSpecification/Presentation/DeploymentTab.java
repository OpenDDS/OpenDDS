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
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.Widget;

public class DeploymentTab extends StructuredViewer {
	// A TableViewer with protected abstract methods
	// exposed so that we can simply delegate to them.
	protected TableViewerDelegate viewer;

	protected Composite control;

	protected GeneratorEditor editor;

	public DeploymentTab( Composite parent) {
		control = new Composite( parent, 0);
		control.setLayout( new GridLayout( 2, true));

		// Left Panel
		Composite panel = new Composite(control, 0);
		GridData gridData = new GridData(SWT.FILL, SWT.FILL, true, true);
		panel.setLayoutData(gridData);
		
		panel.setLayout( new GridLayout( 1, true));
		
		// TODO Put resolved reference information here.

		Label label = new Label(panel, SWT.CENTER);
		label.setText("Deployment Environment Variables");
		label.setLayoutData(new GridData(SWT.CENTER, SWT.TOP, false, false));

		Composite listPane = new Composite(panel,0);
		listPane.setLayoutData( new GridData(SWT.FILL, SWT.FILL, true, true));
		
		listPane.setLayout( new FillLayout(SWT.VERTICAL));
		viewer = new TableViewerDelegate( listPane);
		
		// Right Panel
		panel = new Composite(control, 0);
		gridData = new GridData(SWT.FILL, SWT.FILL, false, true);
		panel.setLayoutData(gridData);

		panel.setLayout( new GridLayout( 2, false));

		label = new Label(panel, SWT.CENTER);
		label.setText("Selection Information");
		
		// TODO Add content here.
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.StructuredViewer#addDragSupport(int, org.eclipse.swt.dnd.Transfer[], org.eclipse.swt.dnd.DragSourceListener)
	 */
	@Override
	public void addDragSupport(int operations, Transfer[] transferTypes, DragSourceListener listener) {
		viewer.addDragSupport(operations, transferTypes, listener);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.StructuredViewer#addDragSupport(int, org.eclipse.swt.dnd.Transfer[], org.eclipse.swt.dnd.DropTargetListener)
	 */
	@Override
	public void addDropSupport(int operations, Transfer[] transferTypes, final DropTargetListener listener) {
		viewer.addDropSupport(operations, transferTypes, listener);
	}

	/*
	 * @see org.eclipse.jface.viewers.Viewer#addSelectionChangedListener()
	 */
	@Override
	public void addSelectionChangedListener(ISelectionChangedListener listener) {
		viewer.addSelectionChangedListener(listener);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.StructuredViewer#doFindInputItem(java.lang.Object)
	 */
	@Override
	protected Widget doFindInputItem(Object element) {
		return viewer.doFindInputItem(element);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.StructuredViewer#doFindItem(java.lang.Object)
	 */
	@Override
	protected Widget doFindItem(Object element) {
		return viewer.doFindItem(element);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.StructuredViewer#doUpdateItem(org.eclipse.swt.widgets.Widget, java.lang.Object, boolean)
	 */
	@Override
	protected void doUpdateItem(Widget item, Object element, boolean fullMap) {
		viewer.doUpdateItem(item, element, fullMap);
	}

	/*
	 * @see org.eclipse.jface.viewers.StructuredViewer#getSelection()
	 */
	@Override
	public ISelection getSelection() {
		return viewer.getSelection();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.StructuredViewer#getSelectionFromWidget()
	 */
	@Override
	protected List<?> getSelectionFromWidget() {
		return viewer.getSelectionFromWidget();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.AbstractTreeViewer#inputChanged()
	 */
	@Override
	protected void inputChanged(Object input, Object oldInput) {
		viewer.inputChanged(input, oldInput);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.StructuredViewer#internalRefresh(java.lang.Object)
	 */
	@Override
	protected void internalRefresh(Object element) {
		viewer.internalRefresh(element);
	}
	
	/*
	 * @see org.eclipse.jface.viewers.Viewer#removeSelectionChangedListener()
	 */
	@Override
    public void removeSelectionChangedListener( ISelectionChangedListener listener) {
    	viewer.removeSelectionChangedListener(listener);
    }

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.StructuredViewer#reveal(java.lang.Object)
	 */
	@Override
	public void reveal(Object element) {
		viewer.reveal( element);

	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.StructuredViewer#setSelectionToWidget(java.util.List, boolean)
	 * 
	 * List<Object>
	 */
	@SuppressWarnings("unchecked")
	@Override
	protected void setSelectionToWidget(List l, boolean reveal) {
		viewer.setSelectionToWidget(l, reveal);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.Viewer#getControl()
	 */
	@Override
	public Control getControl() {
		return control;
	}
	
	public Control getTableControl() {
		return viewer.getControl();
	}

	// From TreeViewer
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.AbstractTreeViewer#setContentProvider(org.eclipse.jface.viewers.IContentProvider)
	 */
	@Override
	public void setContentProvider(IContentProvider provider) {
		viewer.setContentProvider(provider);
	}

	// Cannot override the final StructuredViewer implementation.  This
	// may cause some unforseen issues if code using the StructuredViewer
	// interfaces uses this tab form instead.
	public void setTableInput(Object input) {
		viewer.setInput(input);
	}

	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ColumnViewer#setLabelProvider(org.eclipse.jface.viewers.IBaseLabelProvider)
	 */
	@Override
	public void setLabelProvider(IBaseLabelProvider labelProvider) {
		viewer.setLabelProvider(labelProvider);
	}

	public void setColumnProperties(String[] strings) {
		viewer.setColumnProperties(strings);
	}

	/*
	 * @see org.eclipse.jface.viewers.Viewer#setSelection()
	 */
	@Override
	public void setSelection( ISelection selection) {
		viewer.setSelection( selection);
	}
	
	public Table getTable() {
		return viewer.getTable();
	}
}
