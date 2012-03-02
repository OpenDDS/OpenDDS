package org.opendds.modeling.sdk.model.GeneratorSpecification.Generator;

import java.util.Collection;
import java.util.TreeSet;

import javax.xml.xpath.XPathExpression;
import javax.xml.xpath.XPathExpressionException;

import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

public class ParsedModelFile extends ParsedXmlFile {

	static private final NodeList EMPTY_NODE_LIST = new NodeList() {

		@Override
		public Node item(int index) {
			return null;
		}

		@Override
		public int getLength() {
			return 0;
		}
	};

	private final XPathMatcher nameExpr = new XPathMatcher("//opendds:OpenDDSModel/@name");
	private final XPathMatcher transportNameExpr = new XPathMatcher("//@transportConfig");
	private final XPathMatcher dataLibExpr = new XPathMatcher("//libs[@xsi:type='types:DataLib']");
	private final Collection<String> transportNames = new TreeSet<String>();
	
	private String modelNameCache;
	private Boolean hasDataLibCache;

	/**
	 * Create a new object of the ParsedModelFile class. This is the correct way
	 * to obtain new instances of this class.
	 * 
	 * @param fp
	 *            - IFileProvider to be used by a ParsedModelFile object.
	 * @param eh
	 *            - IErrorHandler to be used by a ParsedModelFile object.
	 * @return - ParsedModelFile object.
	 */
	public static ParsedModelFile create(IFileProvider fp, IErrorHandler eh) {
		return new ParsedModelFile(fp, eh);
	}

	private ParsedModelFile(IFileProvider fp, IErrorHandler eh) {
		super(fp, eh);
	}

	@Override
	public void reset() {
		modelNameCache = null;
		transportNames.clear();
		hasDataLibCache = null;
		super.reset();
	}

	public String getModelName() {
		if (sourceName == null) {
			return null;
		}

		if (modelNameCache != null) {
			return modelNameCache;
		}

		NodeList nodes = nameExpr.retrieve(this);
		if (nodes.getLength() > 0) {
			modelNameCache = nodes.item(0).getNodeValue();
		}
		return modelNameCache;
	}

	public Collection<String> getTransportNames() {
		if (this.sourceName == null) {
			return null;
		}

		if (!transportNames.isEmpty()) {
			return transportNames;
		}

		NodeList nodes = transportNameExpr.retrieve(this);
		for (int index = 0; index < nodes.getLength(); ++index) {
			transportNames.add(nodes.item(index).getNodeValue());
		}
		return transportNames;
	}

	public Boolean hasDataLib() {
		if (this.sourceName == null) {
			return null;
		}

		if (hasDataLibCache != null) {
			return hasDataLibCache;
		}

		NodeList nodes = dataLibExpr.retrieve(this);
		hasDataLibCache = new Boolean(nodes.getLength() > 0);
		return hasDataLibCache;
	}
	
	
	static class XPathMatcher {
		private XPathExpression expr;
		private final String xpath;

		XPathMatcher(String xpath) {
			this.xpath = xpath;
		}

		NodeList retrieve(ParsedXmlFile pxf) {

			if (expr != null) {
				return pxf.parseExpression(expr);
			}

			try {
				expr = pxf.getXpath().compile(xpath);
			} catch (XPathExpressionException e) {
				pxf.errorHandler.error(IErrorHandler.Severity.ERROR,
						"retrieve", "Failed to compile XPath expression: "
								+ xpath, e);
			}
			NodeList nl = pxf.parseExpression(expr);
			return (nl == null) ? EMPTY_NODE_LIST : nl;
		}
	}


}
