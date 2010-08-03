/**
 * 
 */
package org.opendds.modeling.sdk.codegen;

import java.io.BufferedOutputStream;
import java.io.IOException;
import java.net.URI;
import java.net.URL;
import java.util.Iterator;
import java.util.List;

import org.w3c.dom.Document;
import org.w3c.dom.NodeList;
import org.xml.sax.SAXException;

import javax.xml.XMLConstants;
import javax.xml.namespace.NamespaceContext;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.transform.Result;
import javax.xml.transform.Source;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerConfigurationException;
import javax.xml.transform.TransformerException;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.stream.StreamResult;
import javax.xml.transform.stream.StreamSource;
import javax.xml.xpath.XPath;
import javax.xml.xpath.XPathConstants;
import javax.xml.xpath.XPathExpression;
import javax.xml.xpath.XPathExpressionException;
import javax.xml.xpath.XPathFactory;

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
public class OutputsForm extends FormPage implements IDataChangedListener {
	
	private static final String PLUGINNAME = "org.opendds.modeling.sdk";
	private static final String modelNameExpression = "//generator:model/@name";
	private static String generatorNamespace = "http://www.opendds.com/modeling/schemas/Generator/1.0";

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
	private Modelfile modelFile;
	private Targetdir targetDir;
	private Text sourceText;
	private Label idlLabel;
	private Label hLabel;
	private Label cppLabel;
	private Label mpcLabel;
	
	private static XPathFactory pathFactory;
	private static XPath xpath;
	private static XPathExpression nameExpr;
	private String modelName;

	private static TransformerFactory tFactory;
	private static Transformer idlTransformer;
	private static Transformer hTransformer;
	private static Transformer cppTransformer;
	private static Transformer mpcTransformer;

	public OutputsForm(CodeGenForm editor, String id, String title) {
		super(editor, id, title);
		manager = editor.getManager();
		if( tFactory == null) {
			tFactory = TransformerFactory.newInstance();
		}
	}
	
	private XPathExpression getNameExpr() {
		if( nameExpr == null) {
			pathFactory = XPathFactory.newInstance();
			xpath = pathFactory.newXPath();
			xpath.setNamespaceContext( new GeneratorNamespaceContext());
			try {
				nameExpr = xpath.compile(modelNameExpression);
			} catch (XPathExpressionException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
		return nameExpr;
	}
	
	@Override
	public void dataChanged() {
		initialize();
	}
	
	@Override
	protected void createFormContent(IManagedForm managedForm) {
		FormToolkit toolkit = managedForm.getToolkit();
		
		TableWrapLayout layout = new TableWrapLayout();
		layout.numColumns = 2;

		parent = managedForm.getForm().getBody();
		parent.setLayout( layout);

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
				generate(TransformType.IDL);
			}
		});
		idlLabel = toolkit.createLabel(rightPanel, "<model>.idl", SWT.LEFT);
		
		button = toolkit.createButton(rightPanel, "Generate C++ Header", SWT.PUSH);
		TableWrapData hData = new TableWrapData(TableWrapData.FILL);
		button.setLayoutData(hData);
		button.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				generate(TransformType.H);
			}
		});
		hLabel = toolkit.createLabel(rightPanel, "<model>.h", SWT.LEFT);
		
		button = toolkit.createButton(rightPanel, "Generate C++ Body", SWT.PUSH);
		TableWrapData cppData = new TableWrapData(TableWrapData.FILL);
		button.setLayoutData(cppData);
		button.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				generate(TransformType.CPP);
			}
		});
		cppLabel = toolkit.createLabel(rightPanel, "<model>.cpp", SWT.LEFT);
		
		button = toolkit.createButton(rightPanel, "Generate MPC", SWT.PUSH);
		TableWrapData mpcData = new TableWrapData(TableWrapData.FILL);
		button.setLayoutData(mpcData);
		button.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				generate(TransformType.MPC);
			}
		});
		mpcLabel = toolkit.createLabel(rightPanel, "<model>.mpc", SWT.LEFT);
		
		button = toolkit.createButton(rightPanel, "Generate ALL", SWT.PUSH);
		button.addSelectionListener(new SelectionAdapter() {
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
		List<Modelfile> modelList = manager.getModelfiles();
		if( modelList.size() > 0) {
			// Use only the first one if more than one.
			modelFile = modelList.get(0);
			sourceText.setText(modelFile.getName());
		}
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
			manager.updated();
			// TODO Indicate that we are dirty.
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
			manager.setModelName(getModelName());
			modelFile.setName(getSourceName());
			manager.updated();
			// TODO Indicate that we are dirty.
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
		if( modelName != null) {
			return modelName;
		}
		
		if( getNameExpr() == null) {
			ErrorDialog.openError(
					getSite().getShell(),
					"getModelName",
					"XPath expression not available to obtain the model name.",
					new Status(IStatus.ERROR,PLUGINNAME,"Null pointer"));
			return null;
		}

		IWorkspaceRoot workspace = ResourcesPlugin.getWorkspace().getRoot();
		IFile modelFileResource = workspace.getFile(new Path(getSourceName()));
		if(!modelFileResource.exists()) {
			ErrorDialog.openError(
					getSite().getShell(),
					"getModelName",
					"Model file " + getSourceName() + " does not exist.",
					new Status(IStatus.ERROR,PLUGINNAME,"Resource does not exist."));
			return null;
		}
		URI	inputUri = modelFileResource.getLocationURI();
		
		IFile[] modelFile = workspace.findFilesForLocationURI( inputUri);
		if( modelFile.length > 0 && modelFile[0].exists()) {
			// We have a good input file, now parse the XML.
			DocumentBuilderFactory docFactory = DocumentBuilderFactory.newInstance();
			docFactory.setNamespaceAware(true);
			DocumentBuilder builder;
			Document doc;
			try {
				builder = docFactory.newDocumentBuilder();
				doc = builder.parse(modelFile[0].getContents());
				Object result = getNameExpr().evaluate(doc, XPathConstants.NODESET);
				NodeList nodes = (NodeList) result;
				switch(nodes.getLength()) {
				case 1:
					modelName = nodes.item(0).getNodeValue();
					updateModelnameLabels();
					break;
					
				case 0:
					ErrorDialog.openError(
							getSite().getShell(),
							"getModelName",
							"Could not find any model name in the source file " + inputUri.toString(),
							new Status(IStatus.ERROR,PLUGINNAME,"Null pointer"));
					break;
					
					default:
						ErrorDialog.openError(
								getSite().getShell(),
								"getModelName",
								"Found " + nodes.getLength() + " candidate model names in the source file "
								+ inputUri.toString() + ", which is too many!",
								new Status(IStatus.ERROR,PLUGINNAME,"Null pointer"));
						break;
				}

			} catch (ParserConfigurationException e) {
				ErrorDialog.openError(
						getSite().getShell(),
						"getModelName",
						"Problem configuring the parser for file " + inputUri.toString(),
						new Status(IStatus.ERROR,PLUGINNAME,e.getMessage()));
			} catch (SAXException e) {
				ErrorDialog.openError(
						getSite().getShell(),
						"getModelName",
						"Problem parsing the source file " + inputUri.toString(),
						new Status(IStatus.ERROR,PLUGINNAME,e.getMessage()));
			} catch (IOException e) {
				ErrorDialog.openError(
						getSite().getShell(),
						"getModelName",
						"Problem reading the source file " + inputUri.toString(),
						new Status(IStatus.ERROR,PLUGINNAME,e.getMessage()));
			} catch (CoreException e) {
				ErrorDialog.openError(
						getSite().getShell(),
						"getModelName",
						"Problem processing the source file " + inputUri.toString(),
						new Status(IStatus.ERROR,PLUGINNAME,e.getMessage()));
			} catch (XPathExpressionException e) {
				ErrorDialog.openError(
						getSite().getShell(),
						"getModelName",
						"Problem extracting the modelname from the source file " + inputUri.toString(),
						new Status(IStatus.ERROR,PLUGINNAME,e.getMessage()));
			}
		}
		return modelName;
	}
	
	private void updateModelnameLabels() {
		idlLabel.setText(modelName + TransformType.IDL.suffix());
		idlLabel.setSize(idlLabel.computeSize(SWT.DEFAULT, SWT.DEFAULT, true));
		
		hLabel.setText(modelName + TransformType.H.suffix());
		hLabel.setSize(hLabel.computeSize(SWT.DEFAULT, SWT.DEFAULT, true));
		
		cppLabel.setText(modelName + TransformType.CPP.suffix());
		cppLabel.setSize(cppLabel.computeSize(SWT.DEFAULT, SWT.DEFAULT, true));
		
		mpcLabel.setText(modelName + TransformType.MPC.suffix());
		mpcLabel.setSize(mpcLabel.computeSize(SWT.DEFAULT, SWT.DEFAULT, true));
		
		parent.layout(true);
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

		IFile modelFileResource = workspace.getFile(new Path(modelFile.getName()));
		if(!modelFileResource.exists()) {
			ErrorDialog.openError(
					getSite().getShell(),
					which.dialogTitle(),
					"Model file " + modelFile.getName() + " does not exist.",
					new Status(IStatus.ERROR,PLUGINNAME,"Resource does not exist."));
			return;
		}
		
		IFolder targetFolder = workspace.getFolder(new Path(targetDir.getName()));
		if(!targetFolder.exists()) {
			ErrorDialog.openError(
					getSite().getShell(),
					which.dialogTitle(),
					"Target folder " + targetDir.getName() + " does not exist.",
					new Status(IStatus.ERROR,PLUGINNAME,"Resource does not exist."));
			return;
		}

		String modelname = getModelName();
		if( modelname == null) {
			ErrorDialog.openError(
					getSite().getShell(),
					which.dialogTitle(),
					"Unable to generate output for unknown model in source file "
					+ modelFileResource.getLocationURI().toString(),
					new Status(IStatus.ERROR,PLUGINNAME,"Model is not parsable"));
			return;
		}

		URI	inputUri  = modelFileResource.getLocationURI();
		URI	outputUri = targetFolder.getFile(modelname + which.suffix()).getLocationURI();

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

	// This is just annoying.
	public class GeneratorNamespaceContext implements NamespaceContext {
	    public String getNamespaceURI(String prefix) {
	        if (prefix == null) throw new NullPointerException("Null prefix");
	        else if ("generator".equals(prefix)) return generatorNamespace;
	        else if ("xml".equals(prefix)) return XMLConstants.XML_NS_URI;
	        return XMLConstants.NULL_NS_URI;
	    }

	    // This method isn't necessary for XPath processing.
	    public String getPrefix(String uri) {
	        throw new UnsupportedOperationException();
	    }

	    // This method isn't necessary for XPath processing either.
	    @SuppressWarnings("unchecked")
		public Iterator getPrefixes(String uri) {
	        throw new UnsupportedOperationException();
	    }

	}
}
