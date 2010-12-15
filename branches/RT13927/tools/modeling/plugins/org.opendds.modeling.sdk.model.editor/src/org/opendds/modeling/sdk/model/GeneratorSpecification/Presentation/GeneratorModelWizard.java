/**
 * <copyright>
 * </copyright>
 *
 * $Id$
 */
package org.opendds.modeling.sdk.model.GeneratorSpecification.Presentation;


import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import org.eclipse.emf.common.util.URI;

import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.emf.ecore.resource.ResourceSet;

import org.eclipse.emf.ecore.resource.impl.ResourceSetImpl;

import org.eclipse.emf.ecore.EObject;

import org.eclipse.emf.ecore.xmi.XMLResource;

import org.eclipse.emf.edit.ui.provider.ExtendedImageRegistry;

import org.eclipse.core.resources.IContainer;
import org.eclipse.core.resources.IFile;
import org.eclipse.core.resources.IFolder;
import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.IResource;
import org.eclipse.core.resources.ResourcesPlugin;

import org.eclipse.core.runtime.IPath;
import org.eclipse.core.runtime.IProgressMonitor;

import org.eclipse.jface.dialogs.MessageDialog;

import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.jface.viewers.IStructuredSelection;

import org.eclipse.jface.window.Window;
import org.eclipse.jface.wizard.Wizard;
import org.eclipse.jface.wizard.WizardPage;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.INewWizard;
import org.eclipse.ui.IWorkbench;

import org.eclipse.ui.actions.WorkspaceModifyOperation;

import org.eclipse.ui.dialogs.ElementTreeSelectionDialog;
import org.eclipse.ui.dialogs.WizardNewFileCreationPage;

import org.eclipse.ui.model.BaseWorkbenchContentProvider;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.eclipse.ui.part.FileEditorInput;
import org.eclipse.ui.part.ISetSelectionTarget;

import org.opendds.modeling.sdk.model.GeneratorSpecification.CodeGen;
import org.opendds.modeling.sdk.model.GeneratorSpecification.GeneratorFactory;
import org.opendds.modeling.sdk.model.GeneratorSpecification.GeneratorPackage;
import org.opendds.modeling.sdk.model.GeneratorSpecification.Generator.ParsedModelFile;
import org.opendds.modeling.sdk.model.GeneratorSpecification.Generator.SdkGeneratorFactory;
import org.opendds.modeling.sdk.model.GeneratorSpecification.Instance;
import org.opendds.modeling.sdk.model.GeneratorSpecification.Instances;
import org.opendds.modeling.sdk.model.GeneratorSpecification.ModelFile;
import org.opendds.modeling.sdk.model.GeneratorSpecification.TargetDir;
import org.opendds.modeling.sdk.model.GeneratorSpecification.Transport;
import org.opendds.modeling.sdk.model.GeneratorSpecification.TransportOffset;


import org.eclipse.core.runtime.Path;

import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.StructuredSelection;

import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.IWorkbenchPart;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.PartInitException;


/**
 * This is a simple wizard for creating a new model file.
 * <!-- begin-user-doc -->
 * <!-- end-user-doc -->
 * @generated
 */
public class GeneratorModelWizard extends Wizard implements INewWizard {
	/**
	 * The supported extensions for created files.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	public static final List<String> FILE_EXTENSIONS =
		Collections.unmodifiableList(Arrays.asList(GeneratorEditorPlugin.INSTANCE.getString("_UI_GeneratorEditorFilenameExtensions").split("\\s*,\\s*")));

	/**
	 * A formatted list of supported file extensions, suitable for display.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	public static final String FORMATTED_FILE_EXTENSIONS =
		GeneratorEditorPlugin.INSTANCE.getString("_UI_GeneratorEditorFilenameExtensions").replaceAll("\\s*,\\s*", ", ");

	/**
	 * This caches an instance of the model package.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	protected GeneratorPackage generatorPackage = GeneratorPackage.eINSTANCE;

	/**
	 * This caches an instance of the model factory.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	protected GeneratorFactory generatorFactory = generatorPackage.getGeneratorFactory();

	/**
	 * This is the file creation page.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	protected GeneratorModelWizardNewFileCreationPage newFileCreationPage;

	protected GeneratorModelWizardModelSelectionPage modelSelectionPage;

	/**
	 * Remember the selection during initialization for populating the default container.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	protected IStructuredSelection selection;

	/**
	 * Remember the workbench during initialization.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	protected IWorkbench workbench;

	/**
	 * Cache the parsed model file so we only read it once.
	 */
	protected ParsedModelFile parsedModelFile;
	
	/**
	 * This just records the information.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	public void init(IWorkbench workbench, IStructuredSelection selection) {
		this.workbench = workbench;
		this.selection = selection;
		setWindowTitle(GeneratorEditorPlugin.INSTANCE.getString("_UI_Wizard_label"));
		setDefaultPageImageDescriptor(ExtendedImageRegistry.INSTANCE.getImageDescriptor(GeneratorEditorPlugin.INSTANCE.getImage("full/wizban/NewGenerator")));
		parsedModelFile = SdkGeneratorFactory.createParsedModelFile(
				(Window)this.workbench.getActiveWorkbenchWindow());
	}

	/**
	 * Create a new model.
	 * <!-- begin-user-doc -->
	 * These documents are always rooted with a "Code Gen" element.  The
	 * initial model will have the ModelFile, TargetDir, and Instances top
	 * level elements.  Instances will contain a single Instance with the
	 * name 'default', which will have a 'TransportOffset' and one
	 * 'Transport' element for each unique transport index used in the
	 * model.  All of these should be immutable, so that we can then
	 * validate that all other instances have the same transports defined.
	 * <!-- end-user-doc -->
	 * @generated NOT
	 */
	protected EObject createInitialModel() {
		CodeGen codeGen = generatorFactory.createCodeGen();
		
		ModelFile modelFile = generatorFactory.createModelFile();
		String modelFileName = parsedModelFile.getSourceName();
		if( modelFileName != null) {
			modelFile.setName( modelFileName);
		}

		TargetDir targetDir = generatorFactory.createTargetDir();
		URI targetDirURI = modelSelectionPage.getTargetDirURI();
		if( targetDirURI != null && !targetDirURI.isEmpty()) {
			targetDir.setName( targetDirURI.toPlatformString(true));
		}
		codeGen.setTarget(targetDir);

		Instances instances = generatorFactory.createInstances();
		codeGen.setInstances(instances);
		
		Instance instance = generatorFactory.createInstance();
		
		TransportOffset transportOffset = generatorFactory.createTransportOffset();
		transportOffset.setValue(0);
		instance.setTransportOffset(transportOffset);

		if( modelFile.getName() != null) {
			// Load the default instance with a transport for each index found in the model.
			Set<Integer> transportIndices = parsedModelFile.getTransportIds(modelFile.getName());
			for( Integer current : transportIndices) {
				Transport transport = generatorFactory.createTransport();
				transport.setTransportIndex(current);
				instance.getTransport().add(transport);
			}
			codeGen.setSource(modelFile);
		}

		instances.getInstance().add(instance);
		
		return codeGen;
	}

	/**
	 * Do the work after everything is specified.
	 * <!-- begin-user-doc -->
	 * Unconditionally use UTF-8 encoding.  This can be externalized later if it becomes necessary.
	 * <!-- end-user-doc -->
	 * @generated NOT
	 */
	@Override
	public boolean performFinish() {
		try {
			// Remember the file.
			//
			final IFile genFile = getGenFile();

			// Do the work within an operation.
			//
			WorkspaceModifyOperation operation =
				new WorkspaceModifyOperation() {
					@Override
					protected void execute(IProgressMonitor progressMonitor) {
						try {
							// Create a resource set
							//
							ResourceSet resourceSet = new ResourceSetImpl();

							// Get the URI of the model file.
							//
							URI fileURI = URI.createPlatformResourceURI(genFile.getFullPath().toString(), true);

							// Create a resource for this file.
							//
							Resource resource = resourceSet.createResource(fileURI);

							// Add the initial model object to the contents.
							//
							EObject rootObject = createInitialModel();
							if (rootObject != null) {
								resource.getContents().add(rootObject);
							}

							// Save the contents of the resource to the file system.
							//
							Map<Object, Object> options = new HashMap<Object, Object>();
							options.put(XMLResource.OPTION_KEEP_DEFAULT_CONTENT, Boolean.TRUE);
							options.put(XMLResource.OPTION_ENCODING, "UTF-8");
							resource.save(options);
						}
						catch (Exception exception) {
							GeneratorEditorPlugin.INSTANCE.log(exception);
						}
						finally {
							progressMonitor.done();
						}
					}
				};

			getContainer().run(false, false, operation);

			// Select the new file resource in the current view.
			//
			IWorkbenchWindow workbenchWindow = workbench.getActiveWorkbenchWindow();
			IWorkbenchPage page = workbenchWindow.getActivePage();
			final IWorkbenchPart activePart = page.getActivePart();
			if (activePart instanceof ISetSelectionTarget) {
				final ISelection targetSelection = new StructuredSelection(genFile);
				getShell().getDisplay().asyncExec
					(new Runnable() {
						 public void run() {
							 ((ISetSelectionTarget)activePart).selectReveal(targetSelection);
						 }
					 });
			}

			// Open an editor on the new file.
			//
			try {
				page.openEditor
					(new FileEditorInput(genFile),
					 workbench.getEditorRegistry().getDefaultEditor(genFile.getFullPath().toString()).getId());
			}
			catch (PartInitException exception) {
				MessageDialog.openError(workbenchWindow.getShell(), GeneratorEditorPlugin.INSTANCE.getString("_UI_OpenEditorError_label"), exception.getMessage());
				return false;
			}

			return true;
		}
		catch (Exception exception) {
			GeneratorEditorPlugin.INSTANCE.log(exception);
			return false;
		}
	}

	/**
	 * This is the one page of the wizard.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	public class GeneratorModelWizardNewFileCreationPage extends WizardNewFileCreationPage {
		/**
		 * Pass in the selection.
		 * <!-- begin-user-doc -->
		 * <!-- end-user-doc -->
		 * @generated
		 */
		public GeneratorModelWizardNewFileCreationPage(String pageId, IStructuredSelection selection) {
			super(pageId, selection);
		}

		/**
		 * The framework calls this to see if the file is correct.
		 * <!-- begin-user-doc -->
		 * <!-- end-user-doc -->
		 * @generated
		 */
		@Override
		protected boolean validatePage() {
			if (super.validatePage()) {
				String extension = new Path(getFileName()).getFileExtension();
				if (extension == null || !FILE_EXTENSIONS.contains(extension)) {
					String key = FILE_EXTENSIONS.size() > 1 ? "_WARN_FilenameExtensions" : "_WARN_FilenameExtension";
					setErrorMessage(GeneratorEditorPlugin.INSTANCE.getString(key, new Object [] { FORMATTED_FILE_EXTENSIONS }));
					return false;
				}
				return true;
			}
			return false;
		}

		/**
		 * <!-- begin-user-doc -->
		 * <!-- end-user-doc -->
		 * @generated
		 */
		public IFile getGenFile() {
			return ResourcesPlugin.getWorkspace().getRoot().getFile(getContainerFullPath().append(getFileName()));
		}
	}

	/**
	 * @author martinezm
	 *
	 */
	public class GeneratorModelWizardModelSelectionPage extends WizardPage {
		private Text modelFileField;
		private Text targetDirField;
		
		private IPath currentPath;
		
		protected URI targetDirURI;

		public void setCurrentPath(IPath path) {
			currentPath = path;
		}

		/**
		 * @return the targetDirURI
		 */
		public URI getTargetDirURI() {
			return targetDirURI;
		}

		/**
		 * @param targetDirURI the targetDirURI to set
		 */
		public void setTargetDirURI(URI targetDirURI) {
			if( targetDirURI != null && targetDirURI != this.targetDirURI) {
				this.targetDirURI = targetDirURI;
				if( targetDirField != null) {
					targetDirField.setText(targetDirURI.toPlatformString(true));
				}
			}
		}

		/**
		 * @param pageName
		 */
		public GeneratorModelWizardModelSelectionPage(String pageName) {
			super(pageName);
		}

		/**
		 * @param pageName
		 * @param title
		 * @param titleImage
		 */
		public GeneratorModelWizardModelSelectionPage(String pageName,
				String title, ImageDescriptor titleImage) {
			super(pageName, title, titleImage);
		}

		/* (non-Javadoc)
		 * @see org.eclipse.jface.dialogs.IDialogPage#createControl(org.eclipse.swt.widgets.Composite)
		 */
		@Override
		public void createControl(Composite parent) {
			Composite container = new Composite( parent, SWT.NULL);
			final GridLayout gridLayout = new GridLayout();
			gridLayout.numColumns = 3;
			container.setLayout(gridLayout);
			setControl(container);
			
			final Label label_1 = new Label( container, SWT.None);
			final GridData gridData_1 = new GridData();
			gridData_1.horizontalSpan = 3;
			label_1.setLayoutData(gridData_1);
			label_1.setText("Select the file containing the model definition.");
			
			final Label label_2 = new Label( container, SWT.None);
			final GridData gridData_2 = new GridData(GridData.HORIZONTAL_ALIGN_END);
			label_2.setLayoutData(gridData_2);
			label_2.setText("Model File:");
			
			modelFileField = new Text( container, SWT.BORDER);
			modelFileField.addModifyListener( new ModifyListener() {
				@Override
				public void modifyText(ModifyEvent e) {
					updatePageComplete();					
				}
			});
			modelFileField.setLayoutData( new GridData( GridData.FILL_HORIZONTAL));
			String modelFileName = parsedModelFile.getSourceName();
			if( modelFileName != null) {
				modelFileField.setText(modelFileName);
			}
			
			final Button button_1 = new Button( container, SWT.None);
			button_1.addSelectionListener(new SelectionAdapter() {
				public void widgetSelected( SelectionEvent e) {
					browseForModelFile();
				}
			});
			button_1.setText("Browse...");
			
			final Label label_3 = new Label( container, SWT.None);
			final GridData gridData_3 = new GridData();
			gridData_3.horizontalSpan = 3;
			label_3.setLayoutData(gridData_3);
			
			final Label label_4 = new Label( container, SWT.None);
			final GridData gridData_4 = new GridData();
			gridData_4.horizontalSpan = 3;
			label_4.setLayoutData(gridData_4);
			label_4.setText("Select the target directory where generated files will be placed.");
			
			final Label label_5 = new Label( container, SWT.None);
			final GridData gridData_5 = new GridData(GridData.HORIZONTAL_ALIGN_END);
			label_5.setLayoutData(gridData_5);
			label_5.setText("Target Directory:");
			
			targetDirField = new Text( container, SWT.BORDER);
			targetDirField.addModifyListener( new ModifyListener() {
				@Override
				public void modifyText(ModifyEvent e) {
					updatePageComplete();					
				}
			});
			targetDirField.setLayoutData( new GridData( GridData.FILL_HORIZONTAL));
			if( targetDirURI != null) {
				targetDirField.setText(targetDirURI.toPlatformString(true));
			}
			
			final Button button_2 = new Button( container, SWT.None);
			button_2.addSelectionListener(new SelectionAdapter() {
				public void widgetSelected( SelectionEvent e) {
					browseForTargetDir();
				}
			});
			button_2.setText("Browse...");
		}

		protected void browseForTargetDir() {
			// TODO Auto-generated method stub
			
		}

		protected void browseForModelFile() {
			IPath path = browse( new Path(modelFileField.getText()));
			modelFileField.setText(path.toString());
			if( path.segmentCount() > 1) {
				currentPath = path.removeLastSegments(1);
			} else {
				currentPath = null;
			}
		}
		
		private IPath browse( IPath path) {
			ElementTreeSelectionDialog dialog = new ElementTreeSelectionDialog(
					getShell(),
					new WorkbenchLabelProvider(),
					new BaseWorkbenchContentProvider());
			dialog.setTitle("Model Selection");
			dialog.setMessage("Select Model file:");
			dialog.setInput(ResourcesPlugin.getWorkspace().getRoot());
			if( currentPath != null) {
				dialog.setInitialSelection(currentPath.toOSString());
			}
			if(dialog.open() == ElementTreeSelectionDialog.OK) {
				Object[] result = dialog.getResult();
				if( result.length == 1) {
					Object r0 = result[0];
					if( r0 instanceof IFile) {
						return ((IFile)r0).getFullPath();
					}
				}
			}
			return null;
		}

		protected void updatePageComplete() {
			setPageComplete(false);

			String newSource = modelFileField.getText();
			if( newSource == null || newSource.isEmpty()) {
				parsedModelFile.reset();

			} else {
				parsedModelFile.setSourceName( newSource);
				if( !parsedModelFile.exists()) {
					setMessage(null);
					setErrorMessage("Model file "
							+ newSource + " does not exist");
					return;
				}

				String modelName = parsedModelFile.getModelName();
				if( modelName == null) {
					setMessage(null);
					setErrorMessage("Model file "
							+ newSource + " is not a valid OpenDDS model file");
					return;
				}
			}
			setErrorMessage(null);
			setPageComplete(true);
		}

	}

	/**
	 * The framework calls this to create the contents of the wizard.
	 * <!-- begin-user-doc -->
	 * We removed the root and encoding selections as they are hard coded for the editor.
	 * <!-- end-user-doc -->
	 * @generated NOT
	 */
		@Override
	public void addPages() {
		// Create a page, set the title, and the initial generation file name.
		//
		newFileCreationPage = new GeneratorModelWizardNewFileCreationPage("Whatever", selection);
		newFileCreationPage.setTitle(GeneratorEditorPlugin.INSTANCE.getString("_UI_GeneratorModelWizard_label"));
		newFileCreationPage.setDescription(GeneratorEditorPlugin.INSTANCE.getString("_UI_GeneratorModelWizard_description"));
		newFileCreationPage.setFileName(GeneratorEditorPlugin.INSTANCE.getString("_UI_GeneratorEditorFilenameDefaultBase") + "." + FILE_EXTENSIONS.get(0));
		addPage(newFileCreationPage);

		// Page for selecting the model file and target directory.
		//
		modelSelectionPage = new GeneratorModelWizardModelSelectionPage("Model File and Target","Model File and Target",null);
		modelSelectionPage.setTitle(GeneratorEditorPlugin.INSTANCE.getString("_UI_Wizard_selection_page_label"));
		modelSelectionPage.setDescription(GeneratorEditorPlugin.INSTANCE.getString("_UI_Wizard_selection_page_description"));
		addPage(modelSelectionPage);

		// Try and get the resource selection to determine a current directory and model file.
		//
		if (selection != null && !selection.isEmpty()) {
			// Get the resource...
			//
			Object selectedElement = selection.iterator().next();
			if (selectedElement instanceof IResource) {
				// Get the resource parent, if its a file.
				//
				IResource selectedResource = (IResource)selectedElement;
				if (selectedResource.getType() == IResource.FILE) {
					// If a file is selected, that will be used as the model
					// source file, and a new file will be created for this
					parsedModelFile.setSourceName(
									selectedResource
									.getFullPath()
									.toOSString());
					selectedResource = selectedResource.getParent();
				}

				// This gives us a directory...
				//
				if (selectedResource instanceof IFolder || selectedResource instanceof IProject) {
					// Set this for the container.
					//
					newFileCreationPage.setContainerFullPath(selectedResource.getFullPath());
					modelSelectionPage.setCurrentPath( selectedResource.getFullPath());

					// Make up a unique new name here.
					//
					String defaultGenBaseFilename = GeneratorEditorPlugin.INSTANCE.getString("_UI_GeneratorEditorFilenameDefaultBase");
					String defaultGenFilenameExtension = FILE_EXTENSIONS.get(0);
					String genFilename = defaultGenBaseFilename + "." + defaultGenFilenameExtension;
					for (int i = 1; ((IContainer)selectedResource).findMember(genFilename) != null; ++i) {
						genFilename = defaultGenBaseFilename + i + "." + defaultGenFilenameExtension;
					}
					newFileCreationPage.setFileName(genFilename);
				}
			}
		}
	}

	/**
	 * Get the file from the page.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	public IFile getGenFile() {
		return newFileCreationPage.getGenFile();
	}

}
