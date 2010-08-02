package org.opendds.modeling.sdk.codegen;

import org.eclipse.ui.forms.IManagedForm;
import org.eclipse.ui.forms.editor.FormEditor;
import org.eclipse.ui.forms.editor.FormPage;

public class InstanceForm extends FormPage implements IDataChangedListener {

	public InstanceForm(FormEditor editor, String id, String title) {
		super(editor, id, title);
		// TODO Auto-generated constructor stub
	}
	
	@Override
	public void dataChanged() {
//		widgets.refresh();
	}

	@Override
	protected void createFormContent(IManagedForm managedForm) {
		// TODO Auto-generated method stub
		super.createFormContent(managedForm);
	}
	
}
