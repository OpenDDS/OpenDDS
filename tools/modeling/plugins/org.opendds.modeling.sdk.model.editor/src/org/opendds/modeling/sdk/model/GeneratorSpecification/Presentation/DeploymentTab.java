package org.opendds.modeling.sdk.model.GeneratorSpecification.Presentation;

import java.util.List;

import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.StructuredViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Widget;

public class DeploymentTab extends StructuredViewer {
	protected Composite control;

	protected GeneratorEditor editor;

	protected ISelection selection = StructuredSelection.EMPTY;

	public DeploymentTab( Composite parent) {
		control = new Composite( parent, 0);
		control.setLayout( new GridLayout( 2, false));

		// Left Panel
		Composite panel = new Composite(control, 0);
		GridData gridData = new GridData(SWT.FILL, SWT.FILL, true, true);
		panel.setLayoutData(gridData);
		
		panel.setLayout( new GridLayout( 3, false));

		Label label = new Label(panel, SWT.LEFT);
		label.setText("Left Panel");
		
		// Right Panel
		panel = new Composite(control, 0);
		gridData = new GridData(SWT.FILL, SWT.FILL, false, true);
		panel.setLayoutData(gridData);

		panel.setLayout( new GridLayout( 2, false));

		label = new Label(panel, SWT.LEFT);
		label.setText("Right Panel");
	}

	@Override
	public Control getControl() {
		return control;
	}

	@Override
	public void refresh() {
		super.refresh();
	}

	@Override
	public ISelection getSelection() {
		return selection;
	}

	@Override
	public void setSelection(ISelection selection, boolean reveal) {
		this.selection = selection;
	}

	public void setGeneratorEditor(GeneratorEditor generatorEditor) {
		editor = generatorEditor;
	}

	@Override
	protected Widget doFindInputItem(Object element) {
		return null;
	}

	@Override
	protected Widget doFindItem(Object element) {
		return doFindInputItem( element);
	}

	@Override
	protected void doUpdateItem(Widget item, Object element, boolean fullMap) {
		if( !fullMap) {
			return;
		}
	}

	@SuppressWarnings("unchecked")
	@Override
	protected List getSelectionFromWidget() {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	protected void internalRefresh(Object element) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void reveal(Object element) {
		// TODO Auto-generated method stub
		
	}

	@SuppressWarnings("unchecked")
	@Override
	protected void setSelectionToWidget(List l, boolean reveal) {
		// TODO Auto-generated method stub
		
	}
}
