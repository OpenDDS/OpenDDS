package org.opendds.modeling.sdk.model.GeneratorSpecification.Generator;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.net.MalformedURLException;
import java.net.URISyntaxException;
import java.net.URL;
import java.util.Iterator;

import javax.xml.XMLConstants;
import javax.xml.namespace.NamespaceContext;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.transform.OutputKeys;
import javax.xml.transform.Source;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.dom.DOMResult;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;
import javax.xml.xpath.XPath;
import javax.xml.xpath.XPathConstants;
import javax.xml.xpath.XPathExpression;
import javax.xml.xpath.XPathExpressionException;
import javax.xml.xpath.XPathFactory;

import org.opendds.modeling.sdk.model.GeneratorSpecification.Generator.SdkTransformer.TransformType;
import org.w3c.dom.Document;
import org.w3c.dom.NodeList;
import org.xml.sax.SAXException;

public abstract class ParsedXmlFile {

	private static XPathFactory pathFactory;
	private static XPath xpath;
	private Document modelDocument;
	private DocumentBuilder documentBuilder;
	
	protected String sourceName;
	protected IErrorHandler errorHandler;
	protected IFileProvider fileProvider;

	protected ParsedXmlFile(IFileProvider fp, IErrorHandler eh) {
		fileProvider = fp;
		errorHandler = eh;
	}

	private XPathFactory getPathFactory() {
		if( pathFactory == null) {
			pathFactory = XPathFactory.newInstance();
		}
		return pathFactory;
	}

	protected XPath getXpath() {
		if( xpath == null) {
			xpath = getPathFactory().newXPath();
			xpath.setNamespaceContext(new OpenDDSNamespaceContext());
		}
		return xpath;
	}

	public boolean exists() {
		if(sourceName != null) {
			try {
				URL modelUrl = fileProvider.fromWorkspace(sourceName);
				if (modelUrl == null) {
					return false;
				}
				File modelFile = new File(modelUrl.toURI());
				return modelFile.exists();	
			} catch (MalformedURLException e) {
				errorHandler.error(IErrorHandler.Severity.INFO, "exists",
						"Unable to parse URL for file " + sourceName, e);
	
			} catch (URISyntaxException e) {
				errorHandler.error(IErrorHandler.Severity.INFO, "exists",
						"Badly formed URI for file " + sourceName, e);
			}
		}
		return false;
	}

	public String getSourceName() {
		return sourceName;
	}

	public void setSourceName(String sourceName) {
		if( sourceName == null
		 || (sourceName != null && !sourceName.equals(this.sourceName))) {
			reset();
			this.sourceName = sourceName;
		}
	}
	
	/**
	 * Return the contained OpenDDS model in a form suitable for transformation.
	 * 
	 * @param transformer engine for transforming the model into a resolve model.
	 * 
	 * @return Source suitable for transformation.
	 */
	public Source getSource( SdkTransformer transformer) {
		Document doc = getModelDocument();
		if (doc == null) {
			return null; // messages were generated in the get call.
		}

		Source source = new DOMSource( doc);
		if (transformer != null) {
			DOMResult resolved = new DOMResult();
			transformer.transform(TransformType.RESOLVED, source, resolved);

			// for debugging ====>   dumpToFile(resolved);
			
			source = new DOMSource(resolved.getNode());
		}

		return source;
	}

	// for debugging the intermediate result (see "for debugging" comment above)
	@SuppressWarnings("unused")
	private void dumpToFile(DOMResult resolved) {
		try {
			TransformerFactory transfac = TransformerFactory.newInstance();
			Transformer trans = transfac.newTransformer();
			trans.setOutputProperty(OutputKeys.OMIT_XML_DECLARATION, "yes");
			trans.setOutputProperty(OutputKeys.INDENT, "yes");
			FileWriter writer = new FileWriter(new File("temp.xml"));
			StreamResult result = new StreamResult(writer);
			DOMSource temp_source = new DOMSource(resolved.getNode());
			trans.transform(temp_source, result);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	protected Document getModelDocument() {
		if( sourceName == null || sourceName.isEmpty()) {
			return null;
		}
	
		try {
			File modelFile;
			URL modelUrl = fileProvider.fromWorkspace(this.sourceName);
			if (modelUrl != null) {
				modelFile = new File(modelUrl.toURI());
				if (!modelFile.exists()) {
					errorHandler.error(IErrorHandler.Severity.ERROR, "getModelDocument",
							"Model file " + this.sourceName + " does not exist.", null);
					return null;
				}
			} else {
				errorHandler.error(IErrorHandler.Severity.ERROR, "getModelDocument",
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
					errorHandler.error(IErrorHandler.Severity.ERROR, "getModelDocument",
							"Problem configuring the parser for file " + this.sourceName, e);
					return null;
				}
			}
	
			modelDocument = documentBuilder.parse(modelFile);
	
		} catch (SAXException e) {
			errorHandler.error(IErrorHandler.Severity.ERROR, "getModelDocument",
					"Problem parsing the source file " + this.sourceName, e);
		} catch (IOException e) {
			errorHandler.error(IErrorHandler.Severity.ERROR, "getModelDocument",
					"Problem reading the source file " + this.sourceName, e);
		} catch (URISyntaxException e) {
			errorHandler.error(IErrorHandler.Severity.ERROR, "getModelDocument",
					"Problem reading the source file " + this.sourceName, e);
		}
		return modelDocument;
	}

	public void reset() {
		this.sourceName = null;
		this.modelDocument = null;
	}
	
	protected NodeList parseExpression( XPathExpression expression) {
		if( expression == null) {
			return null; // messages were generated earlier.
		}

		Document doc = getModelDocument();
		if (doc == null) {
			return null; // messages were generated in the get call.
		}
		
		try {
			Object result = expression.evaluate(doc, XPathConstants.NODESET);
			return (NodeList) result;

		} catch (XPathExpressionException e) {
			errorHandler.error(IErrorHandler.Severity.ERROR, "Xpath expression",
					"Problem parsing expression in the generator file " + sourceName, e);
		}
		return null;
	}

	/// Get the timestamp for this file or 0 if it's not known 
	public long getTimestamp() {
		try {
			URL modelUrl = fileProvider.fromWorkspace(this.sourceName);
			if (modelUrl == null) {
				return 0;
			}
			File modelFile = new File(modelUrl.toURI());
			return modelFile.lastModified();
		} catch (MalformedURLException e) {
			return 0;
		} catch (URISyntaxException e) {
			return 0;
		}
	}

	public static class OpenDDSNamespaceContext implements NamespaceContext {
		private static final String openDDSNamespace = "http://www.opendds.org/modeling/schemas/OpenDDS/1.0";
		private static final String generatorNamespace = "http://www.opendds.org/modeling/schemas/Generator/1.0";

		public String getNamespaceURI(String prefix) {
	        if (prefix == null) throw new NullPointerException("Null prefix");
	        else if ("opendds".equals(prefix)) return openDDSNamespace;
	        else if ("generator".equals(prefix)) return generatorNamespace;
	        else if ("xml".equals(prefix)) return XMLConstants.XML_NS_URI;
	        else if ("xsi".equals(prefix)) return XMLConstants.W3C_XML_SCHEMA_INSTANCE_NS_URI;
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