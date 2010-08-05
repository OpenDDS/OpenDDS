package org.opendds.modeling.sdk.codegen;

import java.io.BufferedOutputStream;
import java.io.IOException;
import java.net.URI;
import java.net.URL;
import java.util.Iterator;

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
import javax.xml.transform.dom.DOMSource;
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
import org.eclipse.core.resources.IWorkspaceRoot;
import org.eclipse.core.resources.ResourcesPlugin;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.FileLocator;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Platform;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.dialogs.ErrorDialog;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Shell;
import org.w3c.dom.Document;
import org.w3c.dom.NodeList;
import org.xml.sax.SAXException;

public class CodeGenerator {
	private static final String PLUGINNAME = "org.opendds.modeling.sdk";

	private Composite parent;
	
	private static XPathFactory pathFactory;
	private static XPath xpath;
	private static XPathExpression nameExpr;
	private String modelName;
	private Document modelDocument;
	private DocumentBuilder documentBuilder;

	private String sourceName;

	private static TransformerFactory tFactory;
	private static Transformer idlTransformer;
	private static Transformer hTransformer;
	private static Transformer cppTransformer;
	private static Transformer mpcTransformer;

	private static final String modelNameExpression = "//generator:model/@name";
	private static final String generatorNamespace = "http://www.opendds.com/modeling/schemas/Generator/1.0";

	public static enum TransformType {
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
	
	public CodeGenerator() {
		Shell[] shells = Display.getCurrent().getShells();
		if( shells.length == 0) {
			throw new RuntimeException("CodeGenerator: Unable to find display");
		}
		this.parent = shells[0];
	}
	
	private TransformerFactory getTransformerFactory() {
		if( tFactory == null) {
			tFactory = TransformerFactory.newInstance();
		}
		return tFactory;
	}
	
	private XPathExpression getNameExpr() {
		if( nameExpr == null) {
			pathFactory = XPathFactory.newInstance();
			xpath = pathFactory.newXPath();
			xpath.setNamespaceContext( new GeneratorNamespaceContext());
			try {
				nameExpr = xpath.compile(modelNameExpression);
			} catch (XPathExpressionException e) {
				ErrorDialog.openError(
						parent.getShell(),
						"getNameExpr",
						"XPath expression not available to obtain the model name.",
						new Status(IStatus.ERROR,PLUGINNAME,"Null pointer"));
			}
		}
		return nameExpr;
	}
	
	private Document getModelDocument( String sourceName) {
		if( modelDocument != null) {
			return modelDocument;
		}
		
		IWorkspaceRoot workspace = ResourcesPlugin.getWorkspace().getRoot();
		IFile modelFileResource = workspace.getFile(new Path(sourceName));
		if(!modelFileResource.exists()) {
			ErrorDialog.openError(
					parent.getShell(),		// XXX
					"getModelDocument",
					"Model file " + sourceName + " does not exist.",
					new Status(IStatus.ERROR,PLUGINNAME,"Resource does not exist."));
			return null;
		}
		
		// We have a good input file, now parse the XML.
		if( documentBuilder == null) {
			DocumentBuilderFactory docFactory = DocumentBuilderFactory.newInstance();
			docFactory.setNamespaceAware(true);
			try {
				documentBuilder = docFactory.newDocumentBuilder();

			} catch (ParserConfigurationException e) {
				ErrorDialog.openError(
						parent.getShell(),		// XXX
						"getModelDocument",
						"Problem configuring the parser for file " + sourceName,
						new Status(IStatus.ERROR,PLUGINNAME,e.getMessage()));
				return null;
			}
		}

		try {
			modelDocument = documentBuilder.parse(modelFileResource.getContents());

		} catch (SAXException e) {
			ErrorDialog.openError(
					parent.getShell(),		// XXX
					"getModelDocument",
					"Problem parsing the source file " + sourceName,
					new Status(IStatus.ERROR,PLUGINNAME,e.getMessage()));
		} catch (IOException e) {
			ErrorDialog.openError(
					parent.getShell(),		// XXX
					"getModelDocument",
					"Problem reading the source file " + sourceName,
					new Status(IStatus.ERROR,PLUGINNAME,e.getMessage()));
		} catch (CoreException e) {
			ErrorDialog.openError(
					parent.getShell(),		// XXX
					"getModelDocument",
					"Problem processing the source file " + sourceName,
					new Status(IStatus.ERROR,PLUGINNAME,e.getMessage()));
		}
		return modelDocument;
	}

	public String getModelName( String sourceName) {
		if( this.sourceName == sourceName) {
			return modelName;
		}
		this.sourceName = sourceName;
		this.modelDocument = null;

		XPathExpression nameExpr = getNameExpr();
		if( nameExpr == null) {
			return null; // messages were generated in the get call.
		}

		Document doc = getModelDocument( sourceName);
		if( doc == null) {
			return null; // messages were generated in the get call.
		}
		
		try {
			Object result = nameExpr.evaluate(doc, XPathConstants.NODESET);
			NodeList nodes = (NodeList) result;
			switch(nodes.getLength()) {
			case 1:
				modelName = nodes.item(0).getNodeValue();
				break;

			case 0:
				ErrorDialog.openError(
						parent.getShell(),		// XXX
						"getModelName",
						"Could not find any model name in the source file " + sourceName,
						new Status(IStatus.ERROR,PLUGINNAME,"Null pointer"));
				break;

			default:
				ErrorDialog.openError(
						parent.getShell(),		// XXX
						"getModelName",
						"Found " + nodes.getLength() + " candidate model names in the source file "
						+ sourceName + ", which is too many!",
						new Status(IStatus.ERROR,PLUGINNAME,"Null pointer"));
				break;
			}

		} catch (XPathExpressionException e) {
			ErrorDialog.openError(
					parent.getShell(),		// XXX
					"getModelName",
					"Problem extracting the modelname from the source file " + sourceName,
					new Status(IStatus.ERROR,PLUGINNAME,e.getMessage()));
		}
		return modelName;
	}
	
	public void generate( TransformType which, String sourceName, String targetName) {
		if( which.getTransformer() == null) {
			URL url = FileLocator.find(Platform.getBundle(PLUGINNAME), new Path(which.xslFilename()), null);
			try {
				TransformerFactory factory = getTransformerFactory();
				if( factory == null) {
					ErrorDialog.openError(
							parent.getShell(),		// XXX
							which.dialogTitle(),
							"Unable to obtain a transformer factory.",
							new Status(IStatus.ERROR,PLUGINNAME,"Null pointer"));
					return;
				}
				Source converter = new StreamSource( url.openStream());
				which.setTransformer( factory.newTransformer(converter));
			} catch (TransformerConfigurationException e) {
				ErrorDialog.openError(
						parent.getShell(),		// XXX
						which.dialogTitle(),
						"Failed to configure the transformer.",
						new Status(IStatus.ERROR,PLUGINNAME,e.getMessageAndLocation()));
			} catch (IOException e) {
				ErrorDialog.openError(
						parent.getShell(),		// XXX
						which.dialogTitle(),
						"Failed opening XSL file " + url.toString() + " for converter.",
						new Status(IStatus.ERROR,PLUGINNAME,e.getMessage()));
			}
		}

		IWorkspaceRoot workspace = ResourcesPlugin.getWorkspace().getRoot();
		IFolder targetFolder = workspace.getFolder(new Path(targetName));
		if(!targetFolder.exists()) {
			ErrorDialog.openError(
					parent.getShell(),		// XXX
					which.dialogTitle(),
					"Target folder " + targetName + " does not exist.",
					new Status(IStatus.ERROR,PLUGINNAME,"Resource does not exist."));
			return;
		}

		String modelname = getModelName( sourceName);
		if( modelname == null) {
			return; // messages were generated in the get call.
		}

		URI	outputUri = targetFolder.getFile(modelname + which.suffix()).getLocationURI();

		StreamResult result = null;
		try {
			IFileStore fileStore = EFS.getStore(outputUri);
			result = new StreamResult(
					              new BufferedOutputStream(
					      		  fileStore.openOutputStream( EFS.OVERWRITE, null)));
		} catch (CoreException e) {
			ErrorDialog.openError(
					parent.getShell(),		// XXX
					which.dialogTitle(),
					"Unable to open the output file for conversion: " + outputUri.toString(),
					new Status(IStatus.ERROR,PLUGINNAME,e.getMessage()));
			return;
		}
		
		Source modelInput;
		try {
			Document doc = getModelDocument( sourceName);
			if( doc == null) {
				return; // messages were generated in the get call.
			}
			modelInput = new DOMSource(doc);
			which.transform(modelInput, result);
		} catch (TransformerException e) {
			ErrorDialog.openError(
					parent.getShell(),		// XXX
					which.dialogTitle(),
					"Transformation failed!",
					new Status(IStatus.ERROR,PLUGINNAME,e.getMessageAndLocation()));
			return;
		}
		
		try {
			targetFolder.refreshLocal(IFile.DEPTH_ONE, null);
		} catch (CoreException e) {
			ErrorDialog.openError(
					parent.getShell(),		// XXX
					which.dialogTitle(),
					"Unable to refresh output folder " + targetName,
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
