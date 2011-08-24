package org.opendds.modeling.sdk.model.GeneratorSpecification.Generator;

import javax.xml.transform.Source;
import javax.xml.xpath.XPathExpression;
import javax.xml.xpath.XPathExpressionException;

import org.w3c.dom.NodeList;

public class ParsedGeneratorFile extends ParsedXmlFile implements IGeneratorModel {
	private static final String sourceExpression = "//generator:CodeGen/source/@name";
	private static final String targetExpression = "//generator:CodeGen/target/@name";
	private static final String offsetExpression = "//instance/transportOffset";
	private static final String transportExpression = "//instance/transport";

	protected static XPathExpression sourceExpr;
	protected static XPathExpression targetExpr;
	protected static XPathExpression offsetExpr;
	protected static XPathExpression transportExpr;

	/**
	 *  Only needed when there is no EMF model to delegate to.
	 */
	protected String targetDirName;

	/**
	 *  Delegate to EMF implementation if we can.
	 */
	protected IGeneratorModel generatorModel = null;
	
	/**
	 *  This holds the reference to the ParsedModelFile since it 'owns' the model.
	 */
	protected ParsedModelFile parsedModelFile;
	
	protected Source source = null;

	/**
	 * Create a new object of the ParsedGeneratorFile class.
	 * This is the correct way to obtain new instances of this class.
	 * 
	 * @param fp - IFileProvider to be used by a ParsedGeneratorFile object.
	 * @param eh - IErrorHandler to be used by a ParsedGeneratorFile object.
	 * @return   - ParsedGeneratorFile object.
	 */
	public static ParsedGeneratorFile create(IFileProvider fp, IErrorHandler eh) {
		return new ParsedGeneratorFile(fp, eh);
	}

	protected ParsedGeneratorFile(IFileProvider fp, IErrorHandler eh) {
		super(fp, eh);
		parsedModelFile = ParsedModelFile.create(fp, eh);
	}
	
	public void setGeneratorModel( IGeneratorModel generatorModel) {
		if( generatorModel != null) {
			this.generatorModel = generatorModel;
			this.generatorModel.setParsedModelFile(parsedModelFile);
		}
	}
	
	boolean hasOffset() {
 		return parseExpression(getOffsetExpr()).getLength() > 0;
	}
	boolean hasInstanceTransport() {
		return parseExpression(getTransportExpr()).getLength() > 0;
	}
	
	@Override
	public void reset() {
		source = null;
		setTargetDirName( null);
		parsedModelFile.reset();
		super.reset();
	}
	
	@Override
	public void setEditingDomain( Object editingDomain) {
		if( generatorModel != null) {
			generatorModel.setEditingDomain(editingDomain);
		}
	}

	@Override
	public void setParsedModelFile(ParsedModelFile parsedModelFile) {
		if( parsedModelFile != null && parsedModelFile != this.parsedModelFile) {
			this.parsedModelFile = parsedModelFile;
		}
	}
	
	@Override
	public boolean isModelSource( Object element) {
		if( generatorModel != null) {
			return generatorModel.isModelSource(element);
		}
		return false;
	}
	
	@Override
	public boolean isModelTarget( Object element) {
		if( generatorModel != null) {
			return generatorModel.isModelTarget(element);
		}
		return false;
	}
	
	@Override
	public String getModelName() {
		if( generatorModel != null) {
			return generatorModel.getModelName();
		} else {
			return parsedModelFile.getModelName();
		}
	}
	
	@Override
	public String getModelFileName() {
		if( generatorModel != null) {
			return generatorModel.getModelFileName();
		} else {
			return parsedModelFile.getSourceName();
		}
	}
	
	@Override
	public void setModelFileName( String modelFileName) {
		if( generatorModel != null) {
			generatorModel.setModelFileName( modelFileName);
		} else {
			parsedModelFile.setSourceName(modelFileName);
		}
	}
	
	@Override
	public String getTargetDirName() {
		if( generatorModel != null) {
			return generatorModel.getTargetDirName();
		} else {
			return targetDirName;
		}
	}
	
	@Override
	public void setTargetDirName( String targetDirName) {
		if( generatorModel != null) {
			generatorModel.setTargetDirName( targetDirName);

		} else {
			this.targetDirName = targetDirName;
		}
	}

	@Override
	public void setSourceName( String sourceName) {
		super.setSourceName(sourceName);

		if( parsedModelFile.getSourceName() == null) {
			// Parse the file.
		    setModelFileName( resultFromNodeList( parseExpression( getSourceExpr()), "source"));
		}
		
		if( getTargetDirName() == null) {
			// Parse the file.
		    setTargetDirName( resultFromNodeList( parseExpression( getTargetExpr()), "target"));
		}
	}

	@Override
	public Source getSource( SdkTransformer transformer) {
		if( generatorModel != null) {
			// Grab the transformation input from the EMF model if we can.
			return generatorModel.getSource(transformer);

		} else {
			if( transformer != null) {
				// Grab the transformation input from the pre-processed OpenDDS model.
				return parsedModelFile.getSource(transformer);
				
			} else {
				// Grab the transformation input from our own serialized document.
				return super.getSource(transformer);
			}
		}
	}

	@Override
	public long getTimestamp() {
		if (generatorModel != null) {
			return generatorModel.getTimestamp();
		}
		return super.getTimestamp();
	}

	protected XPathExpression getSourceExpr() {
		if (sourceExpr == null) {
			try {
				sourceExpr = getXpath().compile(sourceExpression);

			} catch (XPathExpressionException e) {
				errorHandler.error(IErrorHandler.Severity.ERROR, "getSourceExpr",
						"XPath expression not available to obtain the source .opendds document.", e);
			}
		}
		return sourceExpr;
	}

	protected XPathExpression getTargetExpr() {
		if (targetExpr == null) {
			try {
				targetExpr = getXpath().compile(targetExpression);

			} catch (XPathExpressionException e) {
				errorHandler.error(IErrorHandler.Severity.ERROR, "getTargetExpr",
						"XPath expression not available to obtain the target directory.", e);
			}
		}
		return targetExpr;
	}

	protected XPathExpression getOffsetExpr() {
		if (offsetExpr == null) {
			try {
				offsetExpr = getXpath().compile(offsetExpression);

			} catch (XPathExpressionException e) {
				errorHandler.error(IErrorHandler.Severity.ERROR, "getOffsetExpr",
						"XPath expression not available to obtain the transport offset.", e);
			}
		}
		return offsetExpr;
	}

	protected XPathExpression getTransportExpr() {
		if (transportExpr == null) {
			try {
				transportExpr = getXpath().compile(transportExpression);

			} catch (XPathExpressionException e) {
				errorHandler.error(IErrorHandler.Severity.ERROR, "getOffsetExpr",
						"XPath expression not available to obtain the instance transport.", e);
			}
		}
		return transportExpr;
	}

	protected String resultFromNodeList(NodeList nodes, String attr) {
		switch (nodes.getLength()) {
		case 1:
			return nodes.item(0).getNodeValue();

		case 0:
			errorHandler.error(IErrorHandler.Severity.ERROR, "resultFromNodeList",
					"Could not find any " + attr + " in the file " + this.sourceName,
					null);
			return null;

		default:
			errorHandler.error(IErrorHandler.Severity.ERROR, "resultFromNodeList",
					"Found " + nodes.getLength() + " candidate " + attr + "s in the file "
					+ this.sourceName + ", which is too many!", null);
			return null;
		}
	}
}
