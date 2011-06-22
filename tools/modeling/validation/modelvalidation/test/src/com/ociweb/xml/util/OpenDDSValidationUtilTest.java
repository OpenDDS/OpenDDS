package com.ociweb.xml.util;

import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.fail;

import java.io.File;
import java.io.FileFilter;

import javax.xml.transform.Transformer;

import org.apache.log4j.Logger;
import org.junit.BeforeClass;
import org.junit.Test;

public class OpenDDSValidationUtilTest {
	private static final Logger logger = Logger.getLogger(OpenDDSValidationUtilTest.class);

	static String ddsRoot;
	static String xsdDir;
	static String testsDir;
	static String testDataDir;
	static final String TRANSFORMED_XSD_DIR = "build";
	static final String XSD_FILE = "OpenDDSXMI.xsd";
	
	@BeforeClass
    public static void oneTimeSetUp() {
		String baseDir = "../../../modeling/";
		xsdDir = baseDir + "plugins/org.opendds.modeling.model/model/";
		testsDir = baseDir + "tests/";
		testDataDir = baseDir + "testdata/";
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
			logger.info("XSL = " + xslFile);
			logger.info("XSD = " + xsdFile);
			XMLUtil.transform(xsdFile, xslFile, transformedXsd);
			XMLUtil.transform(transformedXsd, xslFile, idempotentXsd);
		}
		String validatationXsd = "build/idempotent/OpenDDSXMI.xsd";
		File folder = new File(testsDir);
		validate(folder, validatationXsd);
		folder = new File(testDataDir);
		validate(folder, validatationXsd);

	}
	
	@Test
	public void testValidate() throws Exception {
		String xml = "test/data/satellite.opendds";
		final String xsd = TRANSFORMED_XSD_DIR + System.getProperty("file.separator") + XSD_FILE;
		XMLUtil.validate(new File(xsd), new File(xml));
		try {
			xml = "test/data/satellite.opendds";
			XMLUtil.validate(new File("bogus.xsd"), new File(xml));
			fail("expected exception");
		} catch (Exception e) {
			
		}
		File folder = new File(testsDir);
		validate(folder, xsd);
		folder = new File(testDataDir);
		validate(folder, xsd);
	}
	
	void validate(File parent, String xsdFile) throws Exception {
		for (File f: parent.listFiles()) {
			if (f.isDirectory()) {
				validate(f, xsdFile);
			} else if (f.getName().toLowerCase().endsWith(".opendds")) {
				logger.info("Validating " + f.getPath());
				XMLUtil.validate(new File(xsdFile), f);
			}
		}
	}
}