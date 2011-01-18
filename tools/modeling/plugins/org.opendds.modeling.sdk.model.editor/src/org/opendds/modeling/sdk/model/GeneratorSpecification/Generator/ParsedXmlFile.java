package org.opendds.modeling.sdk.model.GeneratorSpecification.Generator;

import java.io.File;
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
import javax.xml.xpath.XPath;
import javax.xml.xpath.XPathFactory;

import org.opendds.modeling.sdk.model.GeneratorSpecification.Generator.SdkGenerator.ErrorHandler;
import org.opendds.modeling.sdk.model.GeneratorSpecification.Generator.SdkGenerator.FileProvider;
import org.opendds.modeling.sdk.model.GeneratorSpecification.Generator.SdkGenerator.ErrorHandler.Severity;
import org.w3c.dom.Document;
import org.xml.sax.SAXException;

public abstract class ParsedXmlFile {

	private static XPathFactory pathFactory;
	private static XPath xpath;
	private Document modelDocument;
	private DocumentBuilder documentBuilder;
	protected String sourceName;
	protected ErrorHandler errorHandler;
	protected FileProvider fileProvider;
	private long timestamp;

	protected ParsedXmlFile(FileProvider fp, ErrorHandler eh) {
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

	public void setSourceName(String sourceName) {
		if( sourceName != null && !sourceName.equals(this.sourceName)) {
			reset();
			this.sourceName = sourceName;
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
			timestamp = modelFile.lastModified();
	
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

	public void reset() {
		this.sourceName = null;
		this.modelDocument = null;
		this.timestamp = 0;
	}
	
	public long getTimestamp() {
		return timestamp;
	}

	public static class OpenDDSNamespaceContext implements NamespaceContext {
		private static final String openDDSNamespace = "http://www.opendds.org/modeling/schemas/OpenDDS/1.0",
			generatorNamespace = "http://www.opendds.org/modeling/schemas/Generator/1.0";

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