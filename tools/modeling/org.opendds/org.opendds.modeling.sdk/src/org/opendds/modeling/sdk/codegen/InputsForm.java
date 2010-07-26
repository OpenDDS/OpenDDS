/**
 * 
 */
package org.opendds.modeling.sdk.codegen;

import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.List;
import org.eclipse.ui.forms.IManagedForm;
import org.eclipse.ui.forms.editor.FormPage;
import org.eclipse.ui.forms.widgets.TableWrapLayout;

/**
 * @author martinezm
 *
 */
public class InputsForm extends FormPage {
	
	private GeneratorManager manager;

	public InputsForm(CodeGenForm editor, String id, String title) {
		super(editor, id, title);
		manager = editor.getManager();
	}

	@Override
	protected void createFormContent(IManagedForm managedForm) {

		Composite parent = getManagedForm().getForm().getBody();
		parent.setLayout(new TableWrapLayout());

		getEditor().getToolkit().createLabel(parent, "These are the files that will be generated into code.");
		
		List list = new List(parent, SWT.BORDER | SWT.V_SCROLL);		
		for( Modelfile modelfile : manager.getModelfiles()) {
			list.add( modelfile.getName());
		}
		super.createFormContent(managedForm);
	}

}
