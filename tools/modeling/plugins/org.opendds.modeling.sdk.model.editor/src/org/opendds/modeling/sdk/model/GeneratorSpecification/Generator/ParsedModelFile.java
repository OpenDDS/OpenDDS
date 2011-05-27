package org.opendds.modeling.sdk.model.GeneratorSpecification.Generator;

import java.util.Set;
import java.util.TreeSet;

import javax.xml.xpath.XPathExpression;
import javax.xml.xpath.XPathExpressionException;

import org.w3c.dom.NodeList;

public class ParsedModelFile extends ParsedXmlFile {
	private static final String modelNameExpression = "//opendds:OpenDDSModel/@name";
	private static final String transportIndexExpression = "//@transportId";
	private static final String dataLibExpression = "//libs[@xsi:type='types:DataLib']";

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
	 * @param fp - IFileProvider to be used by a ParsedModelFile object.
	 * @param eh - IErrorHandler to be used by a ParsedModelFile object.
	 * @return   - ParsedModelFile object.
	 */
	public static ParsedModelFile create(IFileProvider fp, IErrorHandler eh) {
		return new ParsedModelFile(fp, eh);
	}

	private ParsedModelFile(IFileProvider fp, IErrorHandler eh) {
		super(fp, eh);
	}

	@Override
	public void reset() {
		modelName = null;
		transportIndices.clear();
		hasDataLib = null;
		super.reset();
	}

	public String getModelName() {
		if( sourceName == null) {
			return null;
		}

		if( modelName == null) {
			NodeList nodes = parseExpression( getNameExpr());
			if( nodes != null && nodes.getLength() > 0) {
				modelName = nodes.item(0).getNodeValue();
			}
		}

		return modelName;
	}
	
	public Set<Integer> getTransportIds() {
		if( this.sourceName == null) {
			return null;
		}

		if( transportIndices.isEmpty()) {
			NodeList nodes = parseExpression( getTransportIdExpr());
			if( nodes != null) {
				for( int index = 0; index < nodes.getLength(); ++index) {
					transportIndices.add(Integer.valueOf(nodes.item(index).getNodeValue()));
				}
			}
		}

		return transportIndices;
	}

	public Boolean hasDataLib() {
		if( this.sourceName == null) {
			return null;
		}

		if( hasDataLib == null) {
			NodeList nodes = parseExpression( getDataLibExpr());
			if( nodes != null) {
			    hasDataLib = new Boolean(nodes.getLength() > 0);
			}
		}

		return hasDataLib;
	}

	private XPathExpression getNameExpr() {
		if (nameExpr == null) {
			try {
				nameExpr = getXpath().compile(modelNameExpression);

			} catch (XPathExpressionException e) {
				errorHandler.error(IErrorHandler.Severity.ERROR, "getNameExpr",
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
				errorHandler.error(IErrorHandler.Severity.ERROR, "getTransportIdExpr",
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
				errorHandler.error(IErrorHandler.Severity.ERROR, "getDataLibExpr",
						"XPath expression not available to check for DataLibs.", e);
			}
		}
		return dataLibExpr;
	}

}
