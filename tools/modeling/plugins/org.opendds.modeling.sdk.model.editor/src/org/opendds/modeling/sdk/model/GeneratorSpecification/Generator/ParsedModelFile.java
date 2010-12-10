package org.opendds.modeling.sdk.model.GeneratorSpecification.Generator;

import java.io.File;
import java.io.IOException;
import java.net.URISyntaxException;
import java.net.URL;
import java.util.Iterator;
import java.util.LinkedHashSet;
import java.util.Set;

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
	private Set<Integer> transportIndices = new LinkedHashSet<Integer>();

	private Document modelDocument;
	private DocumentBuilder documentBuilder;
	private String sourceName;
	private SdkGenerator.ErrorHandler errorHandler;
	private SdkGenerator.FileProvider fileProvider;
	
	public ParsedModelFile( SdkGenerator.FileProvider fp, SdkGenerator.ErrorHandler eh) {
		fileProvider = fp;
		errorHandler = eh;
	}
	
	private XPathExpression getNameExpr() {
		if (nameExpr == null) {
			pathFactory = XPathFactory.newInstance();
			xpath = pathFactory.newXPath();
			xpath.setNamespaceContext(new OpenDDSNamespaceContext());
			try {
				nameExpr = xpath.compile(modelNameExpression);
			} catch (XPathExpressionException e) {
				errorHandler.error(Severity.ERROR, "getNameExpr",
						"XPath expression not available to obtain the model name.", e);
			}
		}
		return nameExpr;
	}
	
	private XPathExpression getTransportIdExpr() {
		if (transportIdExpr == null) {
			pathFactory = XPathFactory.newInstance();
			xpath = pathFactory.newXPath();
			xpath.setNamespaceContext(new OpenDDSNamespaceContext());
			try {
				transportIdExpr = xpath.compile(transportIndexExpression);
			} catch (XPathExpressionException e) {
				errorHandler.error(Severity.ERROR, "getTransportIdExpr",
						"XPath expression not available to obtain the active transport Id values.", e);
			}
		}
		return transportIdExpr;
	}
	
	public Document getModelDocument(String sourceName) {
		if (modelDocument != null) {
			return modelDocument;
		}

		try {
			URL modelUrl = fileProvider.fromWorkspace(sourceName);
			File modelFile = new File(modelUrl.toURI());
			if (!modelFile.exists()) {
				errorHandler.error(Severity.ERROR, "getModelDocument",
						"Model file " + sourceName + " does not exist.", null);
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
							"Problem configuring the parser for file " + sourceName, e);
					return null;
				}
			}

			modelDocument = documentBuilder.parse(modelFile);

		} catch (SAXException e) {
			errorHandler.error(Severity.ERROR, "getModelDocument",
					"Problem parsing the source file " + sourceName, e);
		} catch (IOException e) {
			errorHandler.error(Severity.ERROR, "getModelDocument",
					"Problem reading the source file " + sourceName, e);
		} catch (URISyntaxException e) {
			errorHandler.error(Severity.ERROR, "getModelDocument",
					"Problem reading the source file " + sourceName, e);
		}
		return modelDocument;
	}

	public String getModelName(String sourceName) {
		if (this.sourceName == sourceName) {
			return modelName;
		}
		this.sourceName = sourceName;
		this.modelDocument = null;
		this.transportIndices.clear();

		XPathExpression nameExpr = getNameExpr();
		if (nameExpr == null) {
			return null; // messages were generated in the get call.
		}

		Document doc = getModelDocument(sourceName);
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
						"Could not find any model name in the source file " + sourceName,
						null);
				break;

			default:
				errorHandler.error(Severity.ERROR, "getModelName",
						"Found " + nodes.getLength() + " candidate model names in the source file "
						+ sourceName + ", which is too many!", null);
				break;
			}

		} catch (XPathExpressionException e) {
			errorHandler.error(Severity.ERROR, "getModelName",
					"Problem extracting the modelname from the source file " + sourceName, e);
		}
		return modelName;
	}
	
	public Set<Integer> getTransportIds() {
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

}
