package com.ociweb.xml.util;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.fail;

import java.io.File;
import java.io.FileFilter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;

import javax.xml.transform.Transformer;

import org.apache.log4j.Logger;
import org.junit.BeforeClass;
import org.junit.Test;
import org.xml.sax.SAXException;
import org.xml.sax.SAXParseException;

public class OpenDDSValidationUtilTest {
	private static final Logger logger = Logger.getLogger(OpenDDSValidationUtilTest.class);


	static final String baseDir = "../../";
	static String xsdDir = baseDir + "plugins/org.opendds.modeling.model/model/";
	static String testsDir = baseDir + "tests/";
	static String testDataDir = baseDir + "testdata/";
	static final String TRANSFORMED_XSD_DIR = "build";
	static final String XSD_FILE = "OpenDDSXMI.xsd";

	static final List<File> knownBadFiles = new ArrayList<File>() {{
		add(new File(testDataDir + "DataLib/DataLibConstraintErrorsTest1.opendds"));
		add(new File("test/data/satellite.opendds"));
		add(new File("test/data/DataLibConstraintErrorsTest1.opendds"));
	}};

	@BeforeClass
    public static void oneTimeSetUp() {
	}

	@Test
	public void testCreateTransformer() throws Exception {
		String xslt = "src/xsl/OpenDDSXMI.xslt";
		Transformer transformer = XMLUtil.createTransformer(new File(xslt));
		assertNotNull(transformer);
		xslt = "src/xsl/TopicsXMI.xslt";
		transformer = XMLUtil.createTransformer(new File(xslt));
		assertNotNull(transformer);
		xslt = "src/xsl/TypesXMI.xslt";
		transformer = XMLUtil.createTransformer(new File(xslt));
		assertNotNull(transformer);
	}

	@Test
	public void testTransform() throws Exception {
		String defaultXsltFilename = "Default.xslt";
		File generatedXsdDir = new File(xsdDir);
		File transformedXsdDir = new File(TRANSFORMED_XSD_DIR);
		File xslDir = new File("src/xsl");
		FileFilter filter = new FileFilter() {
			@Override
			public boolean accept(File file) {
				return file.getName().toLowerCase().endsWith(".xsd");
			}
		};
		for (File xsdFile: generatedXsdDir.listFiles(filter)) {
			String xsdFilename = xsdFile.getName();
			String xslFilename = xsdFilename.replaceAll("(.*)\\.[xX][sS][dD]", "$1.xslt");
			File xslFile = new File(xslDir,xslFilename);
			if (!xslFile.exists()) {
				xslFile = new File(xslDir, defaultXsltFilename);
			}
			File transformedXsd = new File(transformedXsdDir, xsdFilename);
			XMLUtil.transform(xsdFile, xslFile, transformedXsd);
		}
	}

	// this is really a functional test
	@Test
	public void testTransformAndValidate() throws Exception {
		String defaultXsltFilename = "Default.xslt";
		File generatedXsdDir = new File(xsdDir);
		File transformedXsdDir = new File(TRANSFORMED_XSD_DIR);
		File idempotentXsdDir = new File("build/idempotent");
		idempotentXsdDir.mkdirs();

		File xslDir = new File("src/xsl");
		FileFilter filter = new FileFilter() {
			@Override
			public boolean accept(File file) {
				return file.getName().toLowerCase().endsWith(".xsd");
			}
		};
		for (File xsdFile: generatedXsdDir.listFiles(filter)) {
			String xsdFilename = xsdFile.getName();
			String xslFilename = xsdFilename.replaceAll("(.*)\\.[xX][sS][dD]", "$1.xslt");
			File xslFile = new File(xslDir,xslFilename);
			if (!xslFile.exists()) {
				xslFile = new File(xslDir, defaultXsltFilename);
			}
			File transformedXsd = new File(transformedXsdDir, xsdFilename);
			File idempotentXsd = new File(idempotentXsdDir, xsdFilename);
			logger.debug("XSL = " + xslFile);
			logger.debug("XSD = " + xsdFile);
			XMLUtil.transform(xsdFile, xslFile, transformedXsd);
			XMLUtil.transform(transformedXsd, xslFile, idempotentXsd);
		}
		String xsd = "build/idempotent/OpenDDSXMI.xsd";
		File folder = new File(testsDir);
        Collection<File> failures = validate(folder, xsd);
        if (!failures.isEmpty()) {
            fail("validation falied for " + failures);
        }
        folder = new File(testDataDir);
        failures = validate(folder, xsd);
        if (!failures.isEmpty()) {
            fail("validation falied for " + failures);
        }

	}

	@Test
	public void testValidate() throws Exception {
		String xml = "test/data/satellite.opendds";
		final String xsd = TRANSFORMED_XSD_DIR + System.getProperty("file.separator") + XSD_FILE;
		XMLUtil.validate(new File(xsd), new File(xml));
		try {
			XMLUtil.validate(new File("bogus.xsd"), new File(xml));
			fail("expected exception");
		} catch (Exception e) {

		}
		File folder = new File(testsDir);
		Collection<File> failures = validate(folder, xsd);
		if (!failures.isEmpty()) {
			fail("validation falied for " + failures);
		}
		folder = new File(testDataDir);
		failures = validate(folder, xsd);
		if (!failures.isEmpty()) {
			fail("validation falied for " + failures);
		}
	}

	@Test
	public void testValidBadFails() throws SAXException, IOException {
		final String xsd = TRANSFORMED_XSD_DIR + System.getProperty("file.separator") + XSD_FILE;
		for (File badfile : knownBadFiles) {
			List<SAXParseException> errors = XMLUtil.validate(new File(xsd), badfile);
			assertFalse(errors.isEmpty());
		}
	}

	Collection<File> validate(File parent, String xsdFile) throws SAXException, IOException {
		Collection<File> failures = new ArrayList<File>();
		for (File f: parent.listFiles()) {
			if (f.isDirectory()) {
			    failures.addAll(validate(f, xsdFile));
			} else if (f.getName().toLowerCase().endsWith(".opendds")) {
				if (!knownBadFiles.contains(f)) {
					logger.debug("Validating " + f.getPath());
					try {
						List<SAXParseException> errors = XMLUtil.validate(new File(xsdFile), f);
						if (!errors.isEmpty()) {
							logger.error(f + " has the following validation errors:");
							for (SAXParseException se : errors) {
								logger.error("Line " + se.getLineNumber() + " Column " + se.getColumnNumber() + ": " + se.getMessage());
							}
							failures.add(f);
						}
					} catch (SAXException e) {
						logger.error(e);
						throw e;
					} catch (IOException e) {
						logger.error(e);
						throw e;
					}
				}
			}
		}
		return failures;
	}
}