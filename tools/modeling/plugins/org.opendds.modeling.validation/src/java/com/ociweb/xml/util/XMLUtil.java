package com.ociweb.xml.util;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Map;

import javax.xml.XMLConstants;
import javax.xml.transform.OutputKeys;
import javax.xml.transform.Result;
import javax.xml.transform.Source;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerConfigurationException;
import javax.xml.transform.TransformerException;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.stream.StreamResult;
import javax.xml.transform.stream.StreamSource;
import javax.xml.validation.Schema;
import javax.xml.validation.SchemaFactory;
import javax.xml.validation.Validator;

import org.w3c.dom.ls.LSResourceResolver;
import org.xml.sax.ErrorHandler;
import org.xml.sax.SAXException;
import org.xml.sax.SAXParseException;

/**
 * Collection of JAXP-based utilities for executing XSL transformations and
 * XML Schema validations.
 */
public class XMLUtil {
	
	public static final String XSD_DIR = "/xsd";

	/**
	 * @param sourceXML the XML file to transform
	 * @param xslt the XSL stylesheet to use for transformation
	 * @param targetXML the result of XSL transformation
	 * @throws TransformerException if an error occurs configuraing or applying the transformation
	 */
	public static void transform(final File sourceXML, final File xslt, final File targetXML) throws TransformerException {
//		logger.debug("sourceXML = " + sourceXML);
//		logger.debug("xslt = " + xslt);
//		logger.debug("targetXML = " + targetXML);
		Transformer transformer = createTransformer(xslt, Collections.<String,String>emptyMap());
		Source source = new StreamSource(sourceXML);
		Result result = new StreamResult(targetXML);
		transformer.transform(source, result);
	}

	/**
	 * @param schemaFile the XML Schema file to use for validation
	 * @param xmlFile the XML file to validate
	 * @return a list of SaxParseException objects if validation errors occur
	 * @throws SAXException if a parsing error occurs
	 * @throws IOException if a file I/O error occurs
	 */
	public static List<SAXParseException> validate(File schemaFile, File xmlFile) throws SAXException, IOException {
		return validate(schemaFile, xmlFile, null);
	}
	public static List<SAXParseException> validate(File schemaFile, File xmlFile, LSResourceResolver resolver) throws SAXException, IOException {
//		logger.debug("schemaFile = " + schemaFile);
//		logger.debug("xmlFile = " + xmlFile);
		Source schemaSource = new StreamSource(schemaFile);
    	Source xmlSource = new StreamSource(xmlFile);
        return validate(schemaSource, xmlSource, resolver);
	}
	
	public static List<SAXParseException> validate(Source schemaSource, Source xmlSource) throws SAXException, IOException {
		return validate(schemaSource, xmlSource, null);
	}
	public static List<SAXParseException> validate(Source schemaSource, Source xmlSource, LSResourceResolver resolver) throws SAXException, IOException {
		SchemaFactory factory = SchemaFactory.newInstance(XMLConstants.W3C_XML_SCHEMA_NS_URI);
		if (resolver != null) {
			factory.setResourceResolver(resolver);
		}
		Schema schema = factory.newSchema(schemaSource);
		Validator validator = schema.newValidator();
		ValidationErrorHandler veh = new ValidationErrorHandler();
		validator.setErrorHandler(veh);
		validator.validate(xmlSource);
		return veh.errors;
	}
	
	/**
	 * Error handler for XML schema validation. Errors are treated as validation errors to
	 * report, fatal errors are unrecoverable problems, and warnings are logged and
	 * ignored otherwise.
	 */
	static class ValidationErrorHandler implements ErrorHandler {
		final List<SAXParseException> errors = new ArrayList<SAXParseException>();
		@Override
		public void error(SAXParseException exception) throws SAXException {
			errors.add(exception);
//			logger.debug(exception.getMessage());
		}
		@Override
		public void fatalError(SAXParseException exception) throws SAXException {
			// the parser might throw anyway
			exception.printStackTrace();
			throw exception;
		}
		@Override
		public void warning(SAXParseException exception) throws SAXException {
			// these can be safely ignored
//			logger.debug(exception.getMessage());
		}
	}

    static Transformer createTransformer(final File xsltFile) throws TransformerException {
        return createTransformer(xsltFile, Collections.<String,String>emptyMap());
    }
	/**
	 * @param xsltFile The XSLT stylesheet file
	 * @param parms XSL parameters to pass to the stylesheet.
	 * @return the configured XSLT transformer
	 * @throws TransformerException if there is a problem configuring the transformer
	 */
	static Transformer createTransformer(final File xsltFile, final Map<String,String> parms) throws TransformerException {
		try {
			Source xslSource = new StreamSource(xsltFile);
			TransformerFactory transFact = TransformerFactory.newInstance();
			transFact.setAttribute("indent-number", 2);
			Transformer transformer = transFact.newTransformer(xslSource);
			transformer.setOutputProperty(OutputKeys.INDENT, "yes");
			for (String parm : parms.keySet()) {
			    transformer.setParameter(parm, parms.get(parm));
            }
			return transformer;
		} catch (TransformerConfigurationException e) {
			e.printStackTrace();
			throw e;
		}
	}
}
