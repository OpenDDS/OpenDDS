package org.opendds.modeling.sdk.model.GeneratorSpecification.Presentation;

import java.util.List;

import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.emf.edit.command.AddCommand;
import org.eclipse.emf.edit.domain.EditingDomain;
import org.eclipse.jface.viewers.IBaseLabelProvider;
import org.eclipse.jface.viewers.IContentProvider;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.StructuredViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.dnd.DragSourceListener;
import org.eclipse.swt.dnd.DropTargetListener;
import org.eclipse.swt.dnd.Transfer;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.eclipse.swt.widgets.Tree;
import org.eclipse.swt.widgets.Widget;
import org.opendds.modeling.sdk.model.GeneratorSpecification.CodeGen;
import org.opendds.modeling.sdk.model.GeneratorSpecification.GeneratorFactory;
import org.opendds.modeling.sdk.model.GeneratorSpecification.GeneratorPackage;
import org.opendds.modeling.sdk.model.GeneratorSpecification.LocationPath;
import org.opendds.modeling.sdk.model.GeneratorSpecification.LocationVariable;
import org.opendds.modeling.sdk.model.GeneratorSpecification.SearchLocation;
import org.opendds.modeling.sdk.model.GeneratorSpecification.SearchPaths;

public class DeploymentTab extends StructuredViewer {
	// A TableViewer with protected abstract methods
	// exposed so that we can simply delegate to them.
	protected TreeViewerDelegate viewer;
	protected Text variableText;
	protected Text pathText;
	protected Button actionButton;

	protected Composite control;

	protected GeneratorFactory generatorFactory = GeneratorPackage.eINSTANCE.getGeneratorFactory();
	protected GeneratorEditor editor;
	protected SearchPaths searchPaths;

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
		label.setText("Build Search Paths");
		label.setLayoutData(new GridData(SWT.CENTER, SWT.TOP, false, false));

		Composite listPane = new Composite(panel,0);
		listPane.setLayoutData( new GridData(SWT.FILL, SWT.FILL, true, true));
		
		listPane.setLayout( new FillLayout(SWT.VERTICAL));
		viewer = new TreeViewerDelegate( listPane, SWT.FULL_SELECTION);

		// Area below the display list for entering data.
		Composite enterPane = new Composite(panel, 0);
		enterPane.setLayoutData( new GridData(SWT.FILL, SWT.FILL, true, false));
		
		enterPane.setLayout( new GridLayout( 5, false));

		label = new Label(enterPane, SWT.LEFT);
		label.setText("Base Variable:");
		gridData = new GridData(SWT.RIGHT, SWT.CENTER, false, false);
		label.setLayoutData(gridData);

		variableText = new Text(enterPane, SWT.LEFT | SWT.BORDER | SWT.SINGLE);
		gridData = new GridData(SWT.FILL, SWT.CENTER, true, false);
		variableText.setLayoutData(gridData);

		label = new Label(enterPane, SWT.LEFT);
		label.setText("Path:");
		gridData = new GridData(SWT.RIGHT, SWT.CENTER, false, false);
		label.setLayoutData(gridData);

		pathText = new Text(enterPane, SWT.LEFT | SWT.BORDER | SWT.SINGLE);
		gridData = new GridData(SWT.FILL, SWT.CENTER, true, false);
		pathText.setLayoutData(gridData);
		
		actionButton = new Button( enterPane, SWT.PUSH | SWT.LEFT);
		actionButton.setText("Add");
		actionButton.addSelectionListener(new SelectionAdapter() {
        	public void widgetSelected( SelectionEvent e) {
        		addSearchPath( variableText.getText(), pathText.getText());
        	}
		});
		
		// Right Panel
		panel = new Composite(control, 0);
		gridData = new GridData(SWT.FILL, SWT.FILL, false, true);
		panel.setLayoutData(gridData);

		panel.setLayout( new GridLayout( 2, false));

		label = new Label(panel, SWT.CENTER);
		label.setText("Selection Information");
		
		// TODO Add content here.
	}
	
	public void setEditor( GeneratorEditor editor) {
		if( editor == null || editor == this.editor) {
			return;
		}
		this.editor = editor;
		
		// Grab the the model and domain.
		EditingDomain editingDomain = editor.getEditingDomain();
		EList<Resource> resources = editingDomain.getResourceSet().getResources();
		if (resources.size() > 0) {
			Resource resource = resources.get(0);
			EList<EObject> contents = resource.getContents();
			if (contents.size() > 0) {
				EObject root = contents.get(0);
				if (root instanceof CodeGen) {
					CodeGen codeGen = (CodeGen)root;
					searchPaths = codeGen.getSearchPaths();
				}
			}
		}		
	}
	
	protected void addSearchPath( String baseVariable, String relativePath) {
		if( editor == null) {
			return;
		}

		EditingDomain editingDomain = editor.getEditingDomain();
		if( editingDomain != null && searchPaths != null) {

			SearchLocation newPath = generatorFactory.createSearchLocation();
			if( baseVariable != null && !baseVariable.isEmpty()) {
				LocationVariable variable = generatorFactory.createLocationVariable();
				variable.setValue(baseVariable);
				newPath.setVariable(variable);
			}
			
			if( relativePath != null && !relativePath.isEmpty()) {
				LocationPath path = generatorFactory.createLocationPath();
				path.setValue(relativePath);
				newPath.setPath(path);
			}

			editingDomain.getCommandStack().execute(
					AddCommand.create(
							editingDomain,
							searchPaths,
							GeneratorPackage.eINSTANCE.getSearchLocation(),
							newPath
					));
		}
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
	
	public Control getTreeControl() {
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
	
	public Tree getTree() {
		return viewer.getTree();
	}
}
