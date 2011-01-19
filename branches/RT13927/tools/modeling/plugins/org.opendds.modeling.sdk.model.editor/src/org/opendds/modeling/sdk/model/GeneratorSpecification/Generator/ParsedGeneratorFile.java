package org.opendds.modeling.sdk.model.GeneratorSpecification.Generator;

import javax.xml.xpath.XPathConstants;
import javax.xml.xpath.XPathExpression;
import javax.xml.xpath.XPathExpressionException;

import org.opendds.modeling.sdk.model.GeneratorSpecification.Generator.SdkGenerator.ErrorHandler;
import org.opendds.modeling.sdk.model.GeneratorSpecification.Generator.SdkGenerator.FileProvider;
import org.opendds.modeling.sdk.model.GeneratorSpecification.Generator.SdkGenerator.ErrorHandler.Severity;
import org.w3c.dom.Document;
import org.w3c.dom.NodeList;

public class ParsedGeneratorFile extends ParsedXmlFile {
	private static final String sourceExpression = "//generator:CodeGen/source/@name";
	private static final String targetExpression = "//generator:CodeGen/target/@name";

	private static XPathExpression sourceExpr;
	private static XPathExpression targetExpr;

	private String source;
	private String target;

	/**
	 * Create a new object of the ParsedGeneratorFile class.
	 * This is the correct way to obtain new instances of this class.
	 * 
	 * @param fp - FileProvider to be used by a ParsedGeneratorFile object.
	 * @param eh - ErrorHandler to be used by a ParsedGeneratorFile object.
	 * @return   - ParsedGeneratorFile object.
	 */
	public static ParsedGeneratorFile create(FileProvider fp, ErrorHandler eh) {
		return new ParsedGeneratorFile(fp, eh);
	}

	private ParsedGeneratorFile(FileProvider fp, ErrorHandler eh) {
		super(fp, eh);
	}

	public void reset() {
		source = target = null;
		super.reset();
	}

	private XPathExpression getSourceExpr() {
		if (sourceExpr == null) {
			try {
				sourceExpr = getXpath().compile(sourceExpression);

			} catch (XPathExpressionException e) {
				errorHandler.error(Severity.ERROR, "getSourceExpr",
						"XPath expression not available to obtain the source .opendds document.", e);
			}
		}
		return sourceExpr;
	}

	private XPathExpression getTargetExpr() {
		if (targetExpr == null) {
			try {
				targetExpr = getXpath().compile(targetExpression);

			} catch (XPathExpressionException e) {
				errorHandler.error(Severity.ERROR, "getTargetExpr",
						"XPath expression not available to obtain the target directory.", e);
			}
		}
		return targetExpr;
	}

	public String getSource() {
		return getSource(null);
	}

	public String getSource(String sourceName) {
		setSourceName(sourceName);
		if (this.sourceName == null) {
			return null;
		}

		if (this.source != null) {
			return source;
		}

		XPathExpression sourceExpr = getSourceExpr();
		if (sourceExpr == null) {
			return null; // messages were generated in the get call.
		}

		Document doc = getModelDocument();
		if (doc == null) {
			return null; // messages were generated in the get call.
		}
		
		try {
			Object result = sourceExpr.evaluate(doc, XPathConstants.NODESET);
			NodeList nodes = (NodeList) result;
		    source = resultFromNodeList(nodes, "source");

		} catch (XPathExpressionException e) {
			errorHandler.error(Severity.ERROR, "getSource",
					"Problem finding source in the generator file " + this.sourceName, e);
		}
		return source;
	}

	public String getTarget() {
		return getTarget(null);
	}

	public String getTarget(String sourceName) {
		setSourceName(sourceName);
		if (this.sourceName == null) {
			return null;
		}

		if (this.target != null) {
			return target;
		}

		XPathExpression targetExpr = getTargetExpr();
		if (targetExpr == null) {
			return null; // messages were generated in the get call.
		}

		Document doc = getModelDocument();
		if (doc == null) {
			return null; // messages were generated in the get call.
		}
		
		try {
			Object result = targetExpr.evaluate(doc, XPathConstants.NODESET);
			NodeList nodes = (NodeList) result;
		    target = resultFromNodeList(nodes, "target");
		} catch (XPathExpressionException e) {
			errorHandler.error(Severity.ERROR, "getTarget",
					"Problem finding target in the generator file " + this.sourceName, e);
		}
		return target;
	}

	private String resultFromNodeList(NodeList nodes, String attr) {
		switch (nodes.getLength()) {
		case 1:
			return nodes.item(0).getNodeValue();

		case 0:
			errorHandler.error(Severity.ERROR, "resultFromNodeList",
					"Could not find any " + attr + " in the file " + this.sourceName,
					null);
			return null;

		default:
			errorHandler.error(Severity.ERROR, "resultFromNodeList",
					"Found " + nodes.getLength() + " candidate " + attr + "s in the file "
					+ this.sourceName + ", which is too many!", null);
			return null;
		}
	}
}
