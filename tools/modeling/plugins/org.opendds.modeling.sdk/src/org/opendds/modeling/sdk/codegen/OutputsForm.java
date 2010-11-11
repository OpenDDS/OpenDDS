/**
 * 
 */
package org.opendds.modeling.sdk.codegen;

import java.net.MalformedURLException;
import java.net.URL;
import java.util.List;

import org.eclipse.core.resources.IFile;
import org.eclipse.core.resources.IResource;
import org.eclipse.core.resources.IWorkspaceRoot;
import org.eclipse.core.resources.ResourcesPlugin;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.FileLocator;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Platform;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.dialogs.ErrorDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.dialogs.ContainerSelectionDialog;
import org.eclipse.ui.dialogs.ElementTreeSelectionDialog;
import org.eclipse.ui.forms.IManagedForm;
import org.eclipse.ui.forms.editor.FormPage;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.eclipse.ui.forms.widgets.TableWrapData;
import org.eclipse.ui.forms.widgets.TableWrapLayout;
import org.eclipse.ui.model.BaseWorkbenchContentProvider;
import org.eclipse.ui.model.WorkbenchLabelProvider;

/**
 * @author martinezm
 *
 */
public class OutputsForm extends FormPage {
	
	private static final String PLUGINNAME = "org.opendds.modeling.sdk";
	
	private GeneratorManager manager;
	private Composite parent;
	private Text targetText;
	private Text sourceText;
	private Label idlLabel;
	private Label hLabel;
	private Label cppLabel;
	private Label mpcLabel;
	private String modelName;
	private Modelfile modelFile;
	private Targetdir targetDir;
	private boolean inboundChange = false;
	private CodeGenerator generator;

	public OutputsForm(CodeGenEditor editor, String id, String title) {
		super(editor, id, title);
		manager = editor.getManager();
	}
	
	@Override
	protected void createFormContent(IManagedForm managedForm) {
		FormToolkit toolkit = managedForm.getToolkit();
		
		TableWrapLayout layout = new TableWrapLayout();
		layout.numColumns = 2;

		parent = managedForm.getForm().getBody();
		parent.setLayout( layout);
		generator = new CodeGenerator(new CodeGenerator.FileProvider() {	
			@Override
			public URL fromWorkspace(String fileName) throws MalformedURLException {
				IWorkspaceRoot workspace = ResourcesPlugin.getWorkspace().getRoot();
				return workspace.getFile(new Path(fileName)).getLocationURI().toURL();
			}
			@Override
			public URL fromBundle(String fileName) {
				return FileLocator.find(Platform.getBundle(PLUGINNAME), new Path(fileName), null);
			}
			@Override
			public void refresh(String targetFolder) {
				IWorkspaceRoot workspace = ResourcesPlugin.getWorkspace().getRoot();
				try {
					workspace.getFolder(new Path(targetFolder)).refreshLocal(IFile.DEPTH_ONE, null);
				} catch (CoreException e) {
					throw new RuntimeException(e);
				}
			}
		}, new CodeGenerator.ErrorHandler() {
			@Override
			public void error(Severity sev, String title, String message,
					Throwable exception) {
				int stat_sev;
				switch (sev) {
					case ERROR: stat_sev = IStatus.ERROR; break;
					case WARNING: stat_sev = IStatus.WARNING; break;
					case INFO: stat_sev = IStatus.INFO; break;
					default: stat_sev = IStatus.OK;
				}
				ErrorDialog.openError(parent.getShell(), title,
					null /* use the message from Status object */,
					new Status(stat_sev, PLUGINNAME, message, exception));
			}
		});

		Composite leftPanel = toolkit.createComposite(parent);
		Composite rightPanel = toolkit.createComposite(parent);
		
		TableWrapLayout leftLayout = new TableWrapLayout();
		leftLayout.numColumns = 3;
		leftPanel.setLayout(leftLayout);

		TableWrapData leftData = new TableWrapData(TableWrapData.FILL_GRAB);
		leftPanel.setLayoutData(leftData);
		
		TableWrapLayout rightLayout = new TableWrapLayout();
		rightLayout.numColumns = 2;
		rightPanel.setLayout(rightLayout);

		toolkit.createLabel(leftPanel, "Model File: ", SWT.LEFT);
		
		sourceText = toolkit.createText(leftPanel, null, SWT.LEFT | SWT.BORDER | SWT.SINGLE);
		TableWrapData sourceData = new TableWrapData(TableWrapData.FILL_GRAB);
		sourceText.setLayoutData(sourceData);
		sourceText.addModifyListener(new ModifyListener() {
			public void modifyText(ModifyEvent e) {
				sourceChanged();
			}
		});
		
		Button button;

		button = toolkit.createButton(leftPanel, "Browse...", SWT.PUSH | SWT.LEFT);
		button.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				handleSourceBrowse();
			}
		});

		toolkit.createLabel(leftPanel, "Target Folder: ", SWT.LEFT);
		
		targetText = toolkit.createText(leftPanel, null, SWT.LEFT | SWT.BORDER | SWT.SINGLE);
		TableWrapData targetData = new TableWrapData(TableWrapData.FILL_GRAB);
		targetText.setLayoutData(targetData);
		targetText.addModifyListener(new ModifyListener() {
			public void modifyText(ModifyEvent e) {
				targetChanged();
			}
		});

		button = toolkit.createButton(leftPanel, "Browse...", SWT.PUSH);
		button.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				handleTargetBrowse();
			}
		});

		button = toolkit.createButton(rightPanel, "Generate IDL", SWT.PUSH);
		TableWrapData idlData = new TableWrapData(TableWrapData.FILL);
		button.setLayoutData(idlData);
		button.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				generator.generate(CodeGenerator.TransformType.IDL, getSourceName(), getTargetName());
			}
		});
		idlLabel = toolkit.createLabel(rightPanel, "<model>.idl", SWT.LEFT);
		
		button = toolkit.createButton(rightPanel, "Generate C++ Header", SWT.PUSH);
		TableWrapData hData = new TableWrapData(TableWrapData.FILL);
		button.setLayoutData(hData);
		button.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				generator.generate(CodeGenerator.TransformType.H, getSourceName(), getTargetName());
			}
		});
		hLabel = toolkit.createLabel(rightPanel, "<model>.h", SWT.LEFT);
		
		button = toolkit.createButton(rightPanel, "Generate C++ Body", SWT.PUSH);
		TableWrapData cppData = new TableWrapData(TableWrapData.FILL);
		button.setLayoutData(cppData);
		button.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				generator.generate(CodeGenerator.TransformType.CPP, getSourceName(), getTargetName());
			}
		});
		cppLabel = toolkit.createLabel(rightPanel, "<model>.cpp", SWT.LEFT);
		
		button = toolkit.createButton(rightPanel, "Generate MPC", SWT.PUSH);
		TableWrapData mpcData = new TableWrapData(TableWrapData.FILL);
		button.setLayoutData(mpcData);
		button.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				generator.generate(CodeGenerator.TransformType.MPC, getSourceName(), getTargetName());
			}
		});
		mpcLabel = toolkit.createLabel(rightPanel, "<model>.mpc", SWT.LEFT);
		
		button = toolkit.createButton(rightPanel, "Generate ALL", SWT.PUSH);
		button.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				generator.generate(CodeGenerator.TransformType.IDL, getSourceName(), getTargetName());
				generator.generate(CodeGenerator.TransformType.H, getSourceName(), getTargetName());
				generator.generate(CodeGenerator.TransformType.CPP, getSourceName(), getTargetName());
				generator.generate(CodeGenerator.TransformType.MPC, getSourceName(), getTargetName());
			}
		});
		
		super.createFormContent(managedForm);
	}

	public void dataChanged() {
		List<Modelfile> modelList = manager.getModelfiles();
		if( modelList.size() > 0) {
			// Use only the first one if more than one.
			modelFile = modelList.get(0);
			inboundChange = true;
			sourceText.setText(modelFile.getName());
		}
		List<Targetdir> targetList = manager.getTargetdirs();
		if( targetList.size() > 0) {
			// There should be only one.
			targetDir = targetList.get(0);
			inboundChange = true;
			targetText.setText( targetDir.getName());
		}
		inboundChange = false;
	}
	
	private void targetChanged() {
		IResource container = ResourcesPlugin.getWorkspace().getRoot()
							.findMember(new Path(getTargetName()));

		if (getTargetName().length() == 0) {
			updateTarget("Target folder must be specified");
			return;
		}
		if (container == null
				|| (container.getType() & (IResource.PROJECT | IResource.FOLDER)) == 0) {
			updateTarget("Target folder must exist");
			return;
		}
		if (!container.isAccessible()) {
			updateTarget("Target folder must be writable");
			return;
		}
		updateTarget(null);
	}

	private void updateTarget(String message) {
		if( message != null) {
			ErrorDialog.openError(
					getSite().getShell(),
					"Output Updates",
					"Target Folder not updated.",
					new Status(IStatus.WARNING,PLUGINNAME,message));
		} else {
			targetDir.setName(getTargetName());
			if(!inboundChange) {
				manager.fireModelChanged();
			}
		}
	}

	public String getTargetName() {
		return targetText.getText();
	}

	private void sourceChanged() {
		IResource file = ResourcesPlugin.getWorkspace().getRoot()
		.findMember( getSourceName());

		if (getSourceName().length() == 0) {
			updateSource("Model file must be specified");
			return;
		}
		if (file == null
				|| (file.getType() & IResource.FILE) == 0) {
			updateSource("Model file must exist");
			return;
		}
		if (!file.isAccessible()) {
			updateSource("Model file must be readable");
			return;
		}
		updateSource(null);
	}

	private void updateSource(String message) {
		if( message != null) {
			ErrorDialog.openError(
					getSite().getShell(),
					"Model File Updates",
					"Model file not updated.",
					new Status(IStatus.WARNING,PLUGINNAME,message));
		} else {
			modelName = null;
			updateModelnameLabels();
			manager.setModelName(getModelName());
			modelFile.setName(getSourceName());
			if(!inboundChange) {
				manager.fireModelChanged();
			}
		}
	}

	public String getSourceName() {
		return sourceText.getText();
	}
	
	protected void handleSourceBrowse() {
		ElementTreeSelectionDialog dialog = new ElementTreeSelectionDialog(
				parent.getShell(),
				new WorkbenchLabelProvider(),
				new BaseWorkbenchContentProvider());
		dialog.setTitle("Model Selection");
		dialog.setMessage("Select Model file:");
		dialog.setInput(ResourcesPlugin.getWorkspace().getRoot());
		if(dialog.open() == ElementTreeSelectionDialog.OK) {
			Object[] result = dialog.getResult();
			if( result.length == 1) {
				Object r0 = result[0];
				if( r0 instanceof IFile) {
					sourceText.setText(((IFile)r0).getFullPath().toString());
				}
			}
		}
	}

	private void handleTargetBrowse() {
		ContainerSelectionDialog dialog = new ContainerSelectionDialog(
				parent.getShell(), ResourcesPlugin.getWorkspace().getRoot(), false,
				"Select target folder:");
		dialog.setTitle("Target Selection");
		if (dialog.open() == ContainerSelectionDialog.OK) {
			Object[] result = dialog.getResult();
			if (result.length == 1) {
				targetText.setText(((Path)result[0]).toString());
			}
		}
	}

	private String getModelName() {
		if( modelName == null) {
			modelName = generator.getModelName(getSourceName());
			if (modelName.length() == 0) {
				int dotindex = getSourceName().lastIndexOf(".opendds");
				int slashindex = getSourceName().lastIndexOf("/");
				if (dotindex > 0 && slashindex > 0) {
					modelName = getSourceName().substring(slashindex + 1, dotindex);
				}
			}
		}
		return modelName;
	}
	
	private void updateModelnameLabels() {
		String basename = getModelName();
		if( basename == null) {
			basename = "{unnamed}";
		}
		
		idlLabel.setText(basename + CodeGenerator.TransformType.IDL.suffix());
		idlLabel.setSize(idlLabel.computeSize(SWT.DEFAULT, SWT.DEFAULT, true));
		
		hLabel.setText(basename + CodeGenerator.TransformType.H.suffix());
		hLabel.setSize(hLabel.computeSize(SWT.DEFAULT, SWT.DEFAULT, true));
		
		cppLabel.setText(basename + CodeGenerator.TransformType.CPP.suffix());
		cppLabel.setSize(cppLabel.computeSize(SWT.DEFAULT, SWT.DEFAULT, true));
		
		mpcLabel.setText(basename + CodeGenerator.TransformType.MPC.suffix());
		mpcLabel.setSize(mpcLabel.computeSize(SWT.DEFAULT, SWT.DEFAULT, true));
		
		parent.layout(true);
	}
	
}
