/**
 * 
 */
package org.opendds.modeling.sdk.codegen;

import java.io.BufferedOutputStream;
import java.io.IOException;
import java.net.URI;
import java.net.URISyntaxException;
import java.net.URL;
import java.util.List;

import javax.xml.transform.Result;
import javax.xml.transform.Source;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerConfigurationException;
import javax.xml.transform.TransformerException;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.stream.StreamResult;
import javax.xml.transform.stream.StreamSource;

import org.eclipse.core.filesystem.EFS;
import org.eclipse.core.filesystem.IFileStore;
import org.eclipse.core.resources.IFile;
import org.eclipse.core.resources.IFolder;
import org.eclipse.core.resources.IResource;
import org.eclipse.core.resources.IWorkspaceRoot;
import org.eclipse.core.resources.ResourcesPlugin;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.FileLocator;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.dialogs.ErrorDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.dialogs.ContainerSelectionDialog;
import org.eclipse.ui.forms.IManagedForm;
import org.eclipse.ui.forms.editor.FormPage;
import org.eclipse.ui.forms.widgets.TableWrapLayout;

/**
 * @author martinezm
 *
 */
public class OutputsForm extends FormPage implements IDataChangedListener {
	
	private static final String PLUGINNAME = "org.opendds.modeling.sdk";
	
	private static enum TransformType {
		IDL { public Transformer getTransformer() { return idlTransformer; };
		      public void setTransformer( Transformer t) { idlTransformer = t;};
	          public void transform( Source s, Result r) throws TransformerException { idlTransformer.transform(s, r);};
	          public String dialogTitle() { return "Generate IDL";};
		      public String xslFilename() { return "xml/idl.xsl";};
			  public String suffix() { return ".idl"; }
		    },
		H   { public Transformer getTransformer() { return hTransformer; };
	          public void setTransformer( Transformer t) { hTransformer = t;};
	          public void transform( Source s, Result r) throws TransformerException { hTransformer.transform(s, r);};
	          public String dialogTitle() { return "Generate C++ Header";};
		      public String xslFilename() { return "xml/h.xsl";};
			  public String suffix() { return ".h"; }
		    },
		CPP { public Transformer getTransformer() { return cppTransformer; };
	          public void setTransformer( Transformer t) { cppTransformer = t;};
	          public void transform( Source s, Result r) throws TransformerException { cppTransformer.transform(s, r);};
	          public String dialogTitle() { return "Generate C++ Body";};
		      public String xslFilename() { return "xml/cpp.xsl";};
			  public String suffix() { return ".cpp"; }
		    },
		MPC { public Transformer getTransformer() { return mpcTransformer; };
	          public void setTransformer( Transformer t) { mpcTransformer = t;};
		      public void transform( Source s, Result r) throws TransformerException { mpcTransformer.transform(s, r);};
		      public String dialogTitle() { return "Generate MPC";};
		      public String xslFilename() { return "xml/mpc.xsl";};
		  	  public String suffix() { return ".mpc"; }
		    };

		    public abstract Transformer getTransformer();
		    public abstract void setTransformer( Transformer t);
		    public abstract void transform( Source s, Result r) throws TransformerException;
			public abstract String dialogTitle();
		    public abstract String xslFilename();
			public abstract String suffix();
	};
	
	private GeneratorManager manager;
	private Composite parent;
	private Text targetText;
	private Targetdir targetDir;

	private static TransformerFactory tFactory;
	private static Transformer idlTransformer;
	private static Transformer hTransformer;
	private static Transformer cppTransformer;
	private static Transformer mpcTransformer;

	public OutputsForm(CodeGenForm editor, String id, String title) {
		super(editor, id, title);
		manager = editor.getManager();
		tFactory = TransformerFactory.newInstance();
	}
	
	@Override
	public void dataChanged() {
		initialize();
	}
	
	@Override
	protected void createFormContent(IManagedForm managedForm) {
		parent = managedForm.getForm().getBody();
		parent.setLayout(new TableWrapLayout());
		
		Composite composite = new Composite(parent, SWT.BORDER);
		composite.setLayout( new GridLayout( 3, false));

		Label targetLabel = new Label(composite, SWT.LEFT);
		targetLabel.setText("Target Container: ");
		
		targetText = new Text(composite, SWT.LEFT | SWT.BORDER | SWT.SINGLE);
		GridData gd = new GridData(GridData.FILL_HORIZONTAL);
		targetText.setLayoutData(gd);
		targetText.addModifyListener(new ModifyListener() {
			public void modifyText(ModifyEvent e) {
				targetChanged();
			}
		});

		Button button = new Button(composite, SWT.PUSH);
		button.setText("Browse...");
		button.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				handleBrowse();
			}
		});
		
		Composite buttonBar = new Composite(composite, SWT.BORDER);
		gd = new GridData(GridData.FILL_HORIZONTAL);
		gd.horizontalSpan = 3;
		buttonBar.setLayoutData(gd);
		buttonBar.setLayout(new FillLayout(SWT.HORIZONTAL));
		
		Button genIdlButton = new Button(buttonBar, SWT.PUSH);
		genIdlButton.setText("Generate IDL");
		genIdlButton.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				generate(TransformType.IDL);
			}
		});
		
		Button genHButton = new Button(buttonBar, SWT.PUSH);
		genHButton.setText("Generate C++ Header");
		genHButton.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				generate(TransformType.H);
			}
		});
		
		Button genCppButton = new Button(buttonBar, SWT.PUSH);
		genCppButton.setText("Generate C++ Body");
		genCppButton.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				generate(TransformType.CPP);
			}
		});
		
		Button genMpcButton = new Button(buttonBar, SWT.PUSH);
		genMpcButton.setText("Generate MPC");
		genMpcButton.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				generate(TransformType.MPC);
			}
		});
		
		Button genAllButton = new Button(composite, SWT.PUSH);
		genAllButton.setText("Generate All");
		genAllButton.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				generate(TransformType.IDL);
				generate(TransformType.H);
				generate(TransformType.CPP);
				generate(TransformType.MPC);
			}
		});
		
		dataChanged();
		super.createFormContent(managedForm);
	}
	
	private void initialize() {
		List<Targetdir> list = manager.getTargetdirs();
		if( list.size() > 0) {
			// There should be only one.
			targetDir = list.get(0);
			targetText.setText( targetDir.getName());
		}
	}
	
	private void targetChanged() {
		IResource container = ResourcesPlugin.getWorkspace().getRoot()
							.findMember(new Path(getTargetName()));

		if (getTargetName().length() == 0) {
			updateStatus("Target must be specified");
			return;
		}
		if (container == null
				|| (container.getType() & (IResource.PROJECT | IResource.FOLDER)) == 0) {
			updateStatus("Target must exist");
			return;
		}
		if (!container.isAccessible()) {
			updateStatus("Project must be writable");
			return;
		}
		updateStatus(null);
	}

	private void updateStatus(String message) {
		if( message != null) {
			ErrorDialog.openError(
					getSite().getShell(),
					"Output Updates",
					"Target Directory not updated.",
					new Status(IStatus.WARNING,PLUGINNAME,message));
		} else {
			targetDir.setName(getTargetName());
			manager.updated();
			// TODO Indicate that we are dirty.
		}
	}

	public String getTargetName() {
		return targetText.getText();
	}

	private void handleBrowse() {
		ContainerSelectionDialog dialog = new ContainerSelectionDialog(
				parent.getShell(), ResourcesPlugin.getWorkspace().getRoot(), false,
				"Select target container");
		if (dialog.open() == ContainerSelectionDialog.OK) {
			Object[] result = dialog.getResult();
			if (result.length == 1) {
				targetText.setText(((Path)result[0]).toString());
			}
		}
	}
	
	private void generate( TransformType which) {
		if( which.getTransformer() == null) {
			URL url = FileLocator.find(Platform.getBundle(PLUGINNAME), new Path(which.xslFilename()), null);
			try {
				Source converter = new StreamSource( url.openStream());
				which.setTransformer( tFactory.newTransformer(converter));
			} catch (TransformerConfigurationException e) {
				ErrorDialog.openError(
						null,
						which.dialogTitle(),
						"Failed to configure the transformer.",
						new Status(IStatus.ERROR,PLUGINNAME,e.getMessageAndLocation()));
			} catch (IOException e) {
				ErrorDialog.openError(
						null,
						which.dialogTitle(),
						"Failed opening XSL file " + url.toString() + " for converter.",
						new Status(IStatus.ERROR,PLUGINNAME,e.getMessage()));
			}
		}

		IWorkspaceRoot workspace = ResourcesPlugin.getWorkspace().getRoot();
		IFolder targetFolder = workspace.getFolder(new Path(targetDir.getName()));
		if(!targetFolder.exists()) {
			ErrorDialog.openError(
					getSite().getShell(),
					which.dialogTitle(),
					"Target directory " + targetDir.getName() + " does not exist.",
					new Status(IStatus.ERROR,PLUGINNAME,"Resource does not exist."));
			return;
		}
		
		// TODO Extract these from the input form.
		String modelname = "modelname";
		String inputFile = "/home/projects/martinezm/doc/DDS/sdk/tools/modeling/tests/Codegen/aModel.opendds";

		URI inputUri = null;
		URI outputUri = null;
		try {
			inputUri = new URI("file://" + inputFile);
			outputUri = targetFolder.getFile(modelname + which.suffix()).getLocationURI();
		} catch (URISyntaxException e) {
			ErrorDialog.openError(
					getSite().getShell(),
					which.dialogTitle(),
					"URI Syntax problem.",
					new Status(IStatus.ERROR,PLUGINNAME,e.getMessage()));
			return;
		}

		StreamResult result = null;
		try {
			IFileStore fileStore = EFS.getStore(outputUri);
			result = new StreamResult(
					              new BufferedOutputStream(
					      		  fileStore.openOutputStream( EFS.OVERWRITE, null)));
		} catch (CoreException e) {
			ErrorDialog.openError(
					getSite().getShell(),
					which.dialogTitle(),
					"Unable to open the output file for conversion: " + outputUri.toString(),
					new Status(IStatus.ERROR,PLUGINNAME,e.getMessage()));
			return;
		}
		
		IFile[] modelFile = workspace.findFilesForLocationURI( inputUri);
		if( modelFile.length > 0 && modelFile[0].exists()) {
			StreamSource modelInput;
			try {
				modelInput = new StreamSource(modelFile[0].getContents());
				which.transform(modelInput, result);
			} catch (CoreException e) {
				ErrorDialog.openError(
						getSite().getShell(),
						which.dialogTitle(),
						"Unable to open input model file " + outputUri.toString() + " for conversion.",
						new Status(IStatus.ERROR,PLUGINNAME,e.getMessage()));
				return;
			} catch (TransformerException e) {
				ErrorDialog.openError(
						getSite().getShell(),
						which.dialogTitle(),
						"Transformation failed!",
						new Status(IStatus.ERROR,PLUGINNAME,e.getMessageAndLocation()));
				return;
			}
		}
		
		try {
			targetFolder.refreshLocal(IFile.DEPTH_ONE, null);
		} catch (CoreException e) {
			ErrorDialog.openError(
					getSite().getShell(),
					which.dialogTitle(),
					"Unable to refresh output file " + outputUri.toString(),
					new Status(IStatus.WARNING,PLUGINNAME,e.getMessage()));
		}
	}
	
}
