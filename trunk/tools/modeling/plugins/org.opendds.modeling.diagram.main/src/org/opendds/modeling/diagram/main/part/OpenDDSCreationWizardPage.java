/*
 * (c) Copyright Object Computing, Incorporated.  2005,2010.  All rights reserved.
 */
package org.opendds.modeling.diagram.main.part;

import org.eclipse.core.runtime.IPath;
import org.eclipse.core.runtime.Path;
import org.eclipse.emf.common.util.URI;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.osgi.util.NLS;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.dialogs.WizardNewFileCreationPage;

/**
 * @generated
 */
public class OpenDDSCreationWizardPage extends WizardNewFileCreationPage {

	/**
	 * @generated NOT
	 */
	private String fileBaseName;

	/**
	 * @generated
	 */
	private final String fileExtension;

	/**
	 * @generated
	 */
	public OpenDDSCreationWizardPage(String pageName,
			IStructuredSelection selection, String fileExtension) {
		super(pageName, selection);
		this.fileExtension = fileExtension;
	}

	/**
	 * Override to create files with this extension.
	 *
	 * @generated
	 */
	protected String getExtension() {
		return fileExtension;
	}

	/**
	 * @generated
	 */
	public URI getURI() {
		return URI.createPlatformResourceURI(getFilePath().toString(), false);
	}

	/**
	 * @generated
	 */
	protected IPath getFilePath() {
		IPath path = getContainerFullPath();
		if (path == null) {
			path = new Path(""); //$NON-NLS-1$
		}
		String fileName = getFileName();
		if (fileName != null) {
			path = path.append(fileName);
		}
		return path;
	}

	/**
	 * @generated
	 */
	public void createControl(Composite parent) {
		super.createControl(parent);
		setFileName(OpenDDSDiagramEditorUtil.getUniqueFileName(
				getContainerFullPath(), getFileName(), getExtension()));
		setPageComplete(validatePage());
	}

	/**
	 * @generated
	 */
	protected boolean validatePage() {
		if (!super.validatePage()) {
			return false;
		}
		String extension = getExtension();
		if (extension != null
				&& !getFilePath().toString().endsWith("." + extension)) {
			setErrorMessage(NLS
					.bind(Messages.OpenDDSCreationWizardPageExtensionError,
							extension));
			return false;
		}
		return true;
	}

	/**
	 * @generated NOT
	 */
	public void setFileBaseName(String fileBaseName) {
		this.fileBaseName = fileBaseName;
		setFileName(OpenDDSDiagramEditorUtil.getUniqueFileName(
				getContainerFullPath(), fileBaseName, getExtension()));
	}

	/**
	 * @generated NOT
	 */
	private OpenDDSCreationWizard getHostWizard() {
		return (OpenDDSCreationWizard) getWizard();
	}

}
