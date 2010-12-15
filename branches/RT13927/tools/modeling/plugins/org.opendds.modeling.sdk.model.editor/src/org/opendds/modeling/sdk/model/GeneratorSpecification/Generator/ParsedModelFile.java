package org.opendds.modeling.sdk.model.GeneratorSpecification.Generator;

import java.io.File;
import java.io.IOException;
import java.net.MalformedURLException;
import java.net.URISyntaxException;
import java.net.URL;
import java.util.Iterator;
import java.util.Set;
import java.util.TreeSet;

import javax.xml.XMLConstants;
import javax.xml.namespace.NamespaceContext;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.xpath.XPath;
import javax.xml.xpath.XPathConstants;
import javax.xml.xpath.XPathExpression;
import javax.xml.xpath.XPathExpressionException;
import javax.xml.xpath.XPathFactory;

import org.opendds.modeling.sdk.model.GeneratorSpecification.Generator.SdkGenerator.ErrorHandler.Severity;
import org.w3c.dom.Document;
import org.w3c.dom.NodeList;
import org.xml.sax.SAXException;

public class ParsedModelFile {
	private static final String modelNameExpression = "//opendds:OpenDDSModel/@name";
	private static final String transportIndexExpression = "//@transportId";
	private static final String openDDSNamespace = "http://www.opendds.org/modeling/schemas/OpenDDS/1.0";

	private static XPathFactory pathFactory;
	private static XPath xpath;
	private static XPathExpression nameExpr;
	private static XPathExpression transportIdExpr;

	private String modelName;
	private Set<Integer> transportIndices = new TreeSet<Integer>();

	private Document modelDocument;
	private DocumentBuilder documentBuilder;
	private String sourceName;
	private SdkGenerator.ErrorHandler errorHandler;
	private SdkGenerator.FileProvider fileProvider;
	
	/**
	 * Create a new object of the ParsedModelFile class.
	 * This is the correct way to obtain new instances of this class.
	 * 
	 * @param fp - FileProvider to be used by a ParsedModelFile object.
	 * @param eh - ErrorHandler to be used by a ParsedModelFile object.
	 * @return   - ParsedModelFile object.
	 */
	public static ParsedModelFile create( SdkGenerator.FileProvider fp, SdkGenerator.ErrorHandler eh) {
		return new ParsedModelFile( fp, eh);
	}
	
	private ParsedModelFile( SdkGenerator.FileProvider fp, SdkGenerator.ErrorHandler eh) {
		fileProvider = fp;
		errorHandler = eh;
	}

	private XPathFactory getPathFactory() {
		if( pathFactory == null) {
			pathFactory = XPathFactory.newInstance();
		}
		return pathFactory;
	}

	private XPath getXpath() {
		if( xpath == null) {
			xpath = getPathFactory().newXPath();
			xpath.setNamespaceContext(new OpenDDSNamespaceContext());
		}
		return xpath;
	}

	private XPathExpression getNameExpr() {
		if (nameExpr == null) {
			try {
				nameExpr = getXpath().compile(modelNameExpression);

			} catch (XPathExpressionException e) {
				errorHandler.error(Severity.ERROR, "getNameExpr",
						"XPath expression not available to obtain the model name.", e);
			}
		}
		return nameExpr;
	}
	
	private XPathExpression getTransportIdExpr() {
		if (transportIdExpr == null) {
			try {
				transportIdExpr = getXpath().compile(transportIndexExpression);

			} catch (XPathExpressionException e) {
				errorHandler.error(Severity.ERROR, "getTransportIdExpr",
						"XPath expression not available to obtain the active transport Id values.", e);
			}
		}
		return transportIdExpr;
	}
	
	public boolean exists() {
		if(sourceName != null) {
			try {
				URL modelUrl = fileProvider.fromWorkspace(sourceName);
				File modelFile = new File(modelUrl.toURI());
				return modelFile.exists();

			} catch (MalformedURLException e) {
				errorHandler.error(Severity.INFO, "exists",
						"Unable to parse URL for file " + sourceName, e);

			} catch (URISyntaxException e) {
				errorHandler.error(Severity.INFO, "exists",
						"Badly formed URI for file " + sourceName, e);

			}
		}
		return false;
	}
	
	public String getSourceName() {
		return sourceName;
	}
	
	public void setSourceName( String sourceName) {
		if( sourceName != null && sourceName != this.sourceName) {
			this.sourceName = sourceName;
			this.modelName = null;
			this.modelDocument = null;
			this.transportIndices.clear();
		}
	}
	
	public Document getModelDocument() {
		return getModelDocument(null);
	}
	
	public Document getModelDocument(String sourceName) {		
		setSourceName(sourceName);
		if( this.sourceName == null) {
			return null;
		}

		if (modelDocument != null) {
			return modelDocument;
		}

		try {
			URL modelUrl = fileProvider.fromWorkspace(this.sourceName);
			File modelFile = new File(modelUrl.toURI());
			if (!modelFile.exists()) {
				errorHandler.error(Severity.ERROR, "getModelDocument",
						"Model file " + this.sourceName + " does not exist.", null);
				return null;
			}

			// We have a good input file, now parse the XML.
			if (documentBuilder == null) {
				DocumentBuilderFactory docFactory = DocumentBuilderFactory.newInstance();
				docFactory.setNamespaceAware(true);
				try {
					documentBuilder = docFactory.newDocumentBuilder();
	
				} catch (ParserConfigurationException e) {
					errorHandler.error(Severity.ERROR, "getModelDocument",
							"Problem configuring the parser for file " + this.sourceName, e);
					return null;
				}
			}

			modelDocument = documentBuilder.parse(modelFile);

		} catch (SAXException e) {
			errorHandler.error(Severity.ERROR, "getModelDocument",
					"Problem parsing the source file " + this.sourceName, e);
		} catch (IOException e) {
			errorHandler.error(Severity.ERROR, "getModelDocument",
					"Problem reading the source file " + this.sourceName, e);
		} catch (URISyntaxException e) {
			errorHandler.error(Severity.ERROR, "getModelDocument",
					"Problem reading the source file " + this.sourceName, e);
		}
		return modelDocument;
	}
	
	public String getModelName() {
		return getModelName(null);
	}

	public String getModelName(String sourceName) {
		setSourceName(sourceName);
		if( this.sourceName == null) {
			return null;
		}

		if (this.modelName != null) {
			return modelName;
		}

		XPathExpression nameExpr = getNameExpr();
		if (nameExpr == null) {
			return null; // messages were generated in the get call.
		}

		Document doc = getModelDocument();
		if (doc == null) {
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
				errorHandler.error(Severity.ERROR, "getModelName",
						"Could not find any model name in the source file " + this.sourceName,
						null);
				break;

			default:
				errorHandler.error(Severity.ERROR, "getModelName",
						"Found " + nodes.getLength() + " candidate model names in the source file "
						+ this.sourceName + ", which is too many!", null);
				break;
			}

		} catch (XPathExpressionException e) {
			errorHandler.error(Severity.ERROR, "getModelName",
					"Problem extracting the modelname from the source file " + this.sourceName, e);
		}
		return modelName;
	}
	
	public Set<Integer> getTransportIds() {
		return getTransportIds(null);
	}
	
	public Set<Integer> getTransportIds( String sourceName) {
		setSourceName(sourceName);
		if( this.sourceName == null) {
			return null;
		}

		if (!this.transportIndices.isEmpty()) {
			return transportIndices;
		}

		XPathExpression transportIdExpr = getTransportIdExpr();
		if (transportIdExpr == null) {
			return null; // messages were generated in the get call.
		}

		Document doc = getModelDocument(this.sourceName);
		if (doc == null) {
			return null; // messages were generated in the get call.
		}
		
		try {
			Object result = transportIdExpr.evaluate(doc, XPathConstants.NODESET);
			NodeList nodes = (NodeList) result;
			for( int index = 0; index < nodes.getLength(); ++index) {
				transportIndices.add(Integer.valueOf(nodes.item(index).getNodeValue()));
			}

		} catch (XPathExpressionException e) {
			errorHandler.error(Severity.ERROR, "getTransportIds",
					"Problem extracting the transport indices from the source file " + this.sourceName, e);
		}
		return this.transportIndices;
	}

	public class OpenDDSNamespaceContext implements NamespaceContext {
	    public String getNamespaceURI(String prefix) {
	        if (prefix == null) throw new NullPointerException("Null prefix");
	        else if ("opendds".equals(prefix)) return openDDSNamespace;
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

	public void reset() {
		this.sourceName = null;
		this.modelName = null;
		this.modelDocument = null;
		this.transportIndices.clear();
	}

}
