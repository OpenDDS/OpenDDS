/**
 * 
 */
package org.opendds.modeling.sdk.codegen;

import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.ui.forms.IManagedForm;
import org.eclipse.ui.forms.editor.FormPage;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.eclipse.ui.forms.widgets.TableWrapData;
import org.eclipse.ui.forms.widgets.TableWrapLayout;

/**
 * @author martinezm
 *
 */
public class InputsForm extends FormPage implements IDataChangedListener {
	
	private GeneratorManager manager;
	private Composite parent;
	private TableViewer sourceTable;
	private TableColumn sourceColumn;
	private TableColumn hintColumn;
	
	public InputsForm(CodeGenForm editor, String id, String title) {
		super(editor, id, title);
		manager = editor.getManager();
	}
	
	@Override
	public void dataChanged() {
		sourceTable.refresh();
	}

	@Override
	protected void createFormContent(IManagedForm managedForm) {
		FormToolkit toolkit = managedForm.getToolkit();
		parent = managedForm.getForm().getBody();
		parent.setLayout(new TableWrapLayout());

		toolkit.createLabel(parent, "These are the files that will be generated into code.");
		
		sourceTable = new TableViewer( parent, SWT.MULTI | SWT.FULL_SELECTION);
		sourceTable.setContentProvider( manager.getModelfileProvider());
		sourceTable.setLabelProvider( manager.getLabelProvider());
		sourceTable.setInput(manager);
		
		Table table = sourceTable.getTable();
		table.setHeaderVisible(true);
		TableWrapData tableWrapData = new TableWrapData();
		tableWrapData.align = TableWrapData.FILL;
		tableWrapData.grabVertical = true;
		tableWrapData.valign = TableWrapData.FILL;
		tableWrapData.heightHint = table.getItemHeight() * 10;
		table.setLayoutData(tableWrapData);
		
		sourceColumn = new TableColumn(table,SWT.NONE);
		sourceColumn.setText("Model File");
		sourceColumn.setWidth(150);
		
		hintColumn = new TableColumn(table,SWT.NONE);
		hintColumn.setText("Content Type");
		hintColumn.setWidth(150);
		super.createFormContent(managedForm);
	}

}
