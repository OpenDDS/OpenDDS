package com.ociweb.xml.util;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

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

import org.apache.log4j.Logger;
import org.xml.sax.ErrorHandler;
import org.xml.sax.SAXException;
import org.xml.sax.SAXParseException;

public class XMLUtil {
	private static final Logger logger = Logger.getLogger(XMLUtil.class.getName());
	
	public static void transform(final File sourceXML, final File xslt, final File targetXML) throws TransformerException {
		logger.debug("sourceXML = " + sourceXML);
		logger.debug("xslt = " + xslt);
		logger.debug("targetXML = " + targetXML);
		Transformer transformer = createTransformer(xslt);
		Source source = new StreamSource(sourceXML);
		Result result = new StreamResult(targetXML);
		transformer.transform(source, result);
	}
	
	public static List<SAXParseException> validate(final File schemaFile, final File xmlFile) throws SAXException, IOException {
		logger.debug("schemaFile = " + schemaFile);
		logger.debug("xmlFile = " + xmlFile);
		Source schemaSource = new StreamSource(schemaFile);
    	Source xmlSource = new StreamSource(xmlFile);
    	SchemaFactory factory = SchemaFactory.newInstance(XMLConstants.W3C_XML_SCHEMA_NS_URI);
    	Schema schema = factory.newSchema(schemaSource);
    	Validator validator = schema.newValidator();
    	ValidationErrorHandler veh = new ValidationErrorHandler();
    	validator.setErrorHandler(veh);
        validator.validate(xmlSource);
        return veh.errors;
	}
	
	static class ValidationErrorHandler implements ErrorHandler {
		final List<SAXParseException> errors = new ArrayList<SAXParseException>();
		@Override
		public void error(SAXParseException exception) throws SAXException {
			errors.add(exception);
			logger.debug(exception.getMessage());
		}
		@Override
		public void fatalError(SAXParseException exception) throws SAXException {
			// the parser might throw anyway
			logger.error(exception.getMessage());
			throw exception;
		}
		@Override
		public void warning(SAXParseException exception) throws SAXException {
			// these can be safely ignored
			logger.debug(exception.getMessage());
		}
	}
	
	static Transformer createTransformer(final File xsltFile) throws TransformerException {
		try {
			Source xslSource = new StreamSource(xsltFile);
			TransformerFactory transFact = TransformerFactory.newInstance();
			transFact.setAttribute("indent-number", 2);
			Transformer transformer = transFact.newTransformer(xslSource);
			transformer.setOutputProperty(OutputKeys.INDENT, "yes");
			return transformer;
		} catch (TransformerConfigurationException e) {
			logger.error(e.getMessage());
			throw e;
		}
	}
}
