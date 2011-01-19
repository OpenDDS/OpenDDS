package org.opendds.modeling.sdk.model.GeneratorSpecification.Generator;

import java.util.Set;
import java.util.TreeSet;

import javax.xml.xpath.XPathConstants;
import javax.xml.xpath.XPathExpression;
import javax.xml.xpath.XPathExpressionException;

import org.opendds.modeling.sdk.model.GeneratorSpecification.Generator.SdkGenerator.ErrorHandler;
import org.opendds.modeling.sdk.model.GeneratorSpecification.Generator.SdkGenerator.FileProvider;
import org.opendds.modeling.sdk.model.GeneratorSpecification.Generator.SdkGenerator.ErrorHandler.Severity;
import org.w3c.dom.Document;
import org.w3c.dom.NodeList;

public class ParsedModelFile extends ParsedXmlFile {
	private static final String modelNameExpression = "//opendds:OpenDDSModel/@name";
	private static final String transportIndexExpression = "//@transportId";
	private static final String dataLibExpression = "//libs[@xsi:type='opendds:DataLib']";

	private static XPathExpression nameExpr;
	private static XPathExpression transportIdExpr;
	private static XPathExpression dataLibExpr;

	private String modelName;
	private Set<Integer> transportIndices = new TreeSet<Integer>();
	private Boolean hasDataLib;

	/**
	 * Create a new object of the ParsedModelFile class.
	 * This is the correct way to obtain new instances of this class.
	 * 
	 * @param fp - FileProvider to be used by a ParsedModelFile object.
	 * @param eh - ErrorHandler to be used by a ParsedModelFile object.
	 * @return   - ParsedModelFile object.
	 */
	public static ParsedModelFile create(FileProvider fp, ErrorHandler eh) {
		return new ParsedModelFile(fp, eh);
	}

	private ParsedModelFile(FileProvider fp, ErrorHandler eh) {
		super(fp, eh);
	}

	public void reset() {
		this.modelName = null;
		this.transportIndices.clear();
		this.hasDataLib = null;
		super.reset();
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
	
	private XPathExpression getDataLibExpr() {
		if (dataLibExpr == null) {
			try {
				dataLibExpr = getXpath().compile(dataLibExpression);
			} catch (XPathExpressionException e) {
				errorHandler.error(Severity.ERROR, "getDataLibExpr",
						"XPath expression not available to check for DataLibs.", e);
			}
		}
		return dataLibExpr;
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

	public Boolean hasDataLib() {
		return hasDataLib(null);
	}

	public Boolean hasDataLib(String sourceName) {
		setSourceName(sourceName);
		if( this.sourceName == null) {
			return null;
		}

		if (this.hasDataLib != null) {
			return hasDataLib;
		}

		XPathExpression dcpsExpr = getDataLibExpr();
		if (dcpsExpr == null) {
			return null; // messages were generated in the get call.
		}

		Document doc = getModelDocument();
		if (doc == null) {
			return null; // messages were generated in the get call.
		}
		
		try {
			Object result = dcpsExpr.evaluate(doc, XPathConstants.NODESET);
			NodeList nodes = (NodeList) result;
		    hasDataLib = new Boolean(nodes.getLength() > 0);
		} catch (XPathExpressionException e) {
			errorHandler.error(Severity.ERROR, "hasDataLib",
					"Problem finding DataLibs in the source file " + this.sourceName, e);
		}
		return hasDataLib;
	}
}
