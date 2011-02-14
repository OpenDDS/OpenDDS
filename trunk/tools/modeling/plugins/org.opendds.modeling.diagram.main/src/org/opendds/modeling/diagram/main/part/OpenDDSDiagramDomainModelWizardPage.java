package org.opendds.modeling.diagram.main.part;

import org.eclipse.ui.dialogs.WizardNewFileCreationPage;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.wizard.WizardPage;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.VerifyEvent;
import org.eclipse.swt.events.VerifyListener;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.FontData;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.Text;

/**
 * Get from the user any meta-data information about
 * the domain model. Currently this is just the model name.
 */
public class OpenDDSDiagramDomainModelWizardPage extends WizardPage {

	private String modelName = "";
	private Text modelNameControl;

	public OpenDDSDiagramDomainModelWizardPage(String pageName) {
		super(pageName);
	}

	@Override
	public boolean isPageComplete() {
		if (modelName.isEmpty()) {
			return false;
		}
		return super.isPageComplete();
	}

	@Override
	public void createControl(Composite parent) {

		Composite composite = new Composite(parent, SWT.NULL);
		composite.setLayout(new GridLayout(1, false));
		setControl(composite);

		Label modelNameLabel = new Label(composite, SWT.SINGLE);
		modelNameLabel.setText("Enter the Model Name");

		modelNameControl = new Text(composite, SWT.BORDER);
		modelNameControl.setText(modelName);
		GridData data = new GridData(SWT.HORIZONTAL);
		modelNameControl.setLayoutData(data);

		Label modelNameDescription1 = new Label(composite, SWT.SHADOW_IN);
		modelNameDescription1.setText("This is used in generated file names and to name the resulting");
		Label modelNameDescription2 = new Label(composite, SWT.SHADOW_IN);
		modelNameDescription2.setText("shared library and is used as a C++ namespace identifier.");

		// Validate attempt to change model name
		modelNameControl.addVerifyListener(new VerifyListener() {

			@Override
			public void verifyText(VerifyEvent event) {
				String errorMessage = null;
				// Check that text is a valid C++ identifier character.
				if (!event.text.isEmpty() && !event.text.matches("[0-9a-zA-Z_]+")) {
					event.doit = false;
					errorMessage = "C++ namespace identifier can only be an alphanumeric character or _";
				} else {
					// First character cannot be numeric.
					String attrib1Candidate = ((Text) event.widget).getText();
					if (attrib1Candidate.length() == 0 && event.text.matches("[0-9]")) {
						event.doit = false;
						errorMessage = "C++ namespace identifier cannot begin with a digit";
					}
				}
				setErrorMessage(errorMessage);
			}
		});

		// Handle model name being updated
		modelNameControl.addListener(SWT.Modify, new Listener() {
			@Override
			public void handleEvent(Event event) {
				modelName = ((Text) event.widget).getText();
				setPageComplete(!modelName.isEmpty());
				getHostWizard().diagramModelFilePage.setFileBaseName(modelName);
				getHostWizard().domainModelFilePage.setFileBaseName(modelName);
			}
		});
	}

	public String getModelName() {
		return modelName;
	}

	private OpenDDSCreationWizard getHostWizard() {
		return (OpenDDSCreationWizard) getWizard();
	}

}

