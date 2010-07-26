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
public class OutputsForm extends FormPage {
	
	private GeneratorManager manager;

	public OutputsForm(CodeGenForm editor, String id, String title) {
		super(editor, id, title);
		manager = editor.getManager();
	}
	
	@Override
	protected void createFormContent(IManagedForm managedForm) {

		Composite parent = getManagedForm().getForm().getBody();
		parent.setLayout(new TableWrapLayout());
				
		getEditor().getToolkit().createLabel(parent, "This is where the target generated code will go.");
		
		List list = new List(parent, SWT.BORDER | SWT.V_SCROLL);		
		for( Targetdir targets : manager.getTargetdirs()) {
			list.add( targets.getName());
		}
		super.createFormContent(managedForm);
	}
}
