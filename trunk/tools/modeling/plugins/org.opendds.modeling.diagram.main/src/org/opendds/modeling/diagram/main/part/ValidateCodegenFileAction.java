package org.opendds.modeling.diagram.main.part;

import org.eclipse.core.resources.IFile;
import org.eclipse.emf.common.util.URI;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.IObjectActionDelegate;
import org.eclipse.ui.IWorkbenchPart;

/**
 * @generated NOT
 */
public class ValidateCodegenFileAction extends AbstractValidateXMLFileAction {

	@Override
	String getSchemaFilename() {
		return "GeneratorXMI.xsd";
	}
}
