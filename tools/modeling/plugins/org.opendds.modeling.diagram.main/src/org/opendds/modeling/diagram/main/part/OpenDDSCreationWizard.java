/*
 * (c) Copyright Object Computing, Incorporated.  2005,2010.  All rights reserved.
 */
package org.opendds.modeling.diagram.main.part;

import java.lang.reflect.InvocationTargetException;

import org.eclipse.core.commands.ExecutionException;
import org.eclipse.core.commands.operations.OperationHistoryFactory;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.NullProgressMonitor;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.gmf.runtime.diagram.ui.editparts.DiagramEditPart;
import org.eclipse.gmf.runtime.diagram.ui.parts.DiagramEditor;
import org.eclipse.gmf.runtime.emf.type.core.commands.SetValueCommand;
import org.eclipse.gmf.runtime.emf.type.core.requests.SetRequest;
import org.eclipse.gmf.runtime.notation.Diagram;
import org.eclipse.jface.dialogs.ErrorDialog;
import org.eclipse.jface.operation.IRunnableWithProgress;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.wizard.Wizard;
import org.eclipse.ui.IEditorPart;
import org.eclipse.ui.INewWizard;
import org.eclipse.ui.IWorkbench;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.actions.WorkspaceModifyOperation;
import org.opendds.modeling.model.opendds.OpenDDSModel;

/**
 * @generated
 */
public class OpenDDSCreationWizard extends Wizard implements INewWizard {

	/**
	 * @generated
	 */
	private IWorkbench workbench;

	/**
	 * @generated
	 */
	protected IStructuredSelection selection;

	/**
	 * @generated NOT
	 */
	protected OpenDDSDiagramDomainModelWizardPage domainModelPage;

	/**
	 * @generated
	 */
	protected OpenDDSCreationWizardPage diagramModelFilePage;

	/**
	 * @generated
	 */
	protected OpenDDSCreationWizardPage domainModelFilePage;

	/**
	 * @generated
	 */
	protected Resource diagram;

	/**
	 * @generated
	 */
	private boolean openNewlyCreatedDiagramEditor = true;

	/**
	 * @generated
	 */
	public IWorkbench getWorkbench() {
		return workbench;
	}

	/**
	 * @generated
	 */
	public IStructuredSelection getSelection() {
		return selection;
	}

	/**
	 * @generated
	 */
	public final Resource getDiagram() {
		return diagram;
	}

	/**
	 * @generated
	 */
	public final boolean isOpenNewlyCreatedDiagramEditor() {
		return openNewlyCreatedDiagramEditor;
	}

	/**
	 * @generated
	 */
	public void setOpenNewlyCreatedDiagramEditor(
			boolean openNewlyCreatedDiagramEditor) {
		this.openNewlyCreatedDiagramEditor = openNewlyCreatedDiagramEditor;
	}

	/**
	 * @generated NOT
	 */
	public void init(IWorkbench workbench, IStructuredSelection selection) {
		this.workbench = workbench;
		this.selection = selection;
		setWindowTitle(Messages.OpenDDSCreationWizardTitle);
		// Custom code begin
		setDefaultPageImageDescriptor(OpenDDSDiagramEditorPlugin
				.getBundledImageDescriptor("platform:/plugin/org.opendds.modeling.common/icons/full/obj16/OpenDDS.gif")); //$NON-NLS-1$
		// Custom code end
		setNeedsProgressMonitor(true);
	}

	/**
	 * @generated NOT
	 */
	public void addPages() {
		// Custom code begin
		domainModelPage = new OpenDDSDiagramDomainModelWizardPage("DomainModel");
		domainModelPage.setTitle("OpenDDS SDK Model");
		domainModelPage.setDescription("Create a new OpenDDS SDK model");
		addPage(domainModelPage);
		// Custom code end

		diagramModelFilePage = new OpenDDSCreationWizardPage(
				"DiagramModelFile", getSelection(), "opendds_diagram"); //$NON-NLS-1$ //$NON-NLS-2$
		diagramModelFilePage
				.setTitle(Messages.OpenDDSCreationWizard_DiagramModelFilePageTitle);
		diagramModelFilePage
				.setDescription(Messages.OpenDDSCreationWizard_DiagramModelFilePageDescription);
		addPage(diagramModelFilePage);

		domainModelFilePage = new OpenDDSCreationWizardPage(
				"DomainModelFile", getSelection(), "opendds") { //$NON-NLS-1$ //$NON-NLS-2$

			public void setVisible(boolean visible) {
				if (visible) {
					String fileName = diagramModelFilePage.getFileName();
					fileName = fileName.substring(0, fileName.length()
							- ".opendds_diagram".length()); //$NON-NLS-1$
					setFileName(OpenDDSDiagramEditorUtil.getUniqueFileName(
							getContainerFullPath(), fileName, "opendds")); //$NON-NLS-1$
				}
				super.setVisible(visible);
			}
		};
		domainModelFilePage
				.setTitle(Messages.OpenDDSCreationWizard_DomainModelFilePageTitle);
		domainModelFilePage
				.setDescription(Messages.OpenDDSCreationWizard_DomainModelFilePageDescription);
		addPage(domainModelFilePage);
	}

	/**
	 * @generated NOT
	 */
	public boolean performFinish() {
		IRunnableWithProgress op = new WorkspaceModifyOperation(null) {

			protected void execute(IProgressMonitor monitor)
					throws CoreException, InterruptedException {
				diagram = OpenDDSDiagramEditorUtil.createDiagram(
						diagramModelFilePage.getURI(),
						domainModelFilePage.getURI(), monitor);
				if (isOpenNewlyCreatedDiagramEditor() && diagram != null) {
					try {
						OpenDDSDiagramEditorUtil.openDiagram(diagram);
					} catch (PartInitException e) {
						ErrorDialog.openError(getContainer().getShell(),
								Messages.OpenDDSCreationWizardOpenEditorError,
								null, e.getStatus());
					}
				}
			}
		};
		try {
			getContainer().run(false, true, op);
		} catch (InterruptedException e) {
			return false;
		} catch (InvocationTargetException e) {
			if (e.getTargetException() instanceof CoreException) {
				ErrorDialog.openError(getContainer().getShell(),
						Messages.OpenDDSCreationWizardCreationError, null,
						((CoreException) e.getTargetException()).getStatus());
			} else {
				OpenDDSDiagramEditorPlugin.getInstance().logError(
						"Error creating diagram", e.getTargetException()); //$NON-NLS-1$
			}
			return false;
		}
		// Custom code begin
		modelNameSet(domainModelPage.getModelName());
		// Custom code end
		return diagram != null;
	}

	/**
	 * Populate OpenDDSModel.name attribute.
	 * This must be called after getContainer().run() is called in performFinish()
	 * which creates the DiagramEditPart.
	 * The OpenDDSModel object diagram.contents.element created from
	 * OpenDDSDiagramEditorUtil.createDiagram() is not the same model
	 * behind the DiagramEditPart.
	 * generated NOT
	 */
	private static void modelNameSet(String modelName) {

		///// Get the OpenDDSModel object /////

		IWorkbenchPage page = PlatformUI.getWorkbench()
				.getActiveWorkbenchWindow().getActivePage();
		IEditorPart activeEditor = page.getActiveEditor();
		DiagramEditor diagramEditor = (DiagramEditor) activeEditor;
		DiagramEditPart diagramEditPart = diagramEditor.getDiagramEditPart();
		Object model = diagramEditPart.getModel();
		Diagram diagramObj = (Diagram) model;
		OpenDDSModel modelObj = (OpenDDSModel) diagramObj.getElement();

		///// Set the OpenDDSModel name attribute /////

		SetRequest request = new SetRequest(modelObj, modelObj.eClass()
				.getEStructuralFeature("name"), modelName);
		SetValueCommand command = new SetValueCommand(request);
		try {
			OperationHistoryFactory.getOperationHistory().execute(command,
					new NullProgressMonitor(), null);
		} catch (ExecutionException e) {
			OpenDDSDiagramEditorPlugin.getInstance().logError(
					"Unable to set model name", e); //$NON-NLS-1$
		}
		page.saveEditor(diagramEditor, false);
	}

}
