package org.opendds.modeling.sdk.codegen;

import java.awt.Color;

import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.forms.IManagedForm;
import org.eclipse.ui.forms.editor.FormEditor;
import org.eclipse.ui.forms.editor.FormPage;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.opendds.modeling.sdk.InstanceTraitsContentProvider;
import org.opendds.modeling.sdk.presentation.InstancetraitsEditorPlugin;

public class InstanceForm extends FormPage {

	public InstanceForm(FormEditor editor, String id, String title) {
		super(editor, id, title);
		// TODO Auto-generated constructor stub
	}

	@Override
	protected void createFormContent(IManagedForm managedForm) {
		super.createFormContent(managedForm);
		
		Composite parent = managedForm.getForm().getBody();
		FormToolkit toolkit = managedForm.getToolkit();
		Composite leftPanel = toolkit.createComposite(parent);
		TreeViewer treeViewer = new TreeViewer(parent);
		treeViewer.setContentProvider(new InstanceTraitsContentProvider());
		treeViewer.setInput(null);

	}
	
}
