package org.opendds.modeling.validation;

import java.io.File;
import java.io.FileFilter;

import javax.xml.transform.TransformerException;

import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.CommandLineParser;
import org.apache.commons.cli.HelpFormatter;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;
import org.apache.commons.cli.PosixParser;

import com.ociweb.xml.util.XMLUtil;

public class OpenDDSXMISchemaTransformer {

	public static final String SRC_DIR_ARG = "src";
	public static final String DEST_DIR_ARG = "dest";
	public static final String XSL_DIR = "src/xsl";
	public static final String DEFAULT_XSL_FILE = "Default.xslt";

	public static void main(String[] args) {
		CommandLineParser parser = new PosixParser();
		Options options = new Options();
		options.addOption(SRC_DIR_ARG, true, "source directory for opendds schema files");
		options.addOption(DEST_DIR_ARG, true, "destination directory for corrected schema files");

		try {
			CommandLine cmdLine = parser.parse(options, args);

			if (!(cmdLine.hasOption(SRC_DIR_ARG) && cmdLine.hasOption(DEST_DIR_ARG))) {
				HelpFormatter formatter = new HelpFormatter();
				formatter.printHelp(OpenDDSXMISchemaTransformer.class.getSimpleName(), options);
				System.exit(-1);
			}
			File srcDir = new File(cmdLine.getOptionValue(SRC_DIR_ARG));
			if (!validDir(srcDir)) {
				System.err.println("The directory specified for '" + SRC_DIR_ARG + "' does not appear to be valid");
				System.exit(-1);
			}
			File destDir = new File(cmdLine.getOptionValue(DEST_DIR_ARG));
			if (!validDir(destDir)) {
				System.err.println("The directory specified for '" + DEST_DIR_ARG + "' does not appear to be valid");
				System.exit(-1);
			}
			transform(srcDir, destDir);
		} catch (ParseException  e) {
			e.printStackTrace();
		} catch (TransformerException e) {
			e.printStackTrace();
		}
	}

	static boolean validDir(final File dir) {
		if (!dir.exists() || !dir.isDirectory()) {
			return false;
		}
		return true;
	}

	static void transform(final File sourceDir, final File destDir) throws TransformerException {
//		logger.debug("source dir = " + sourceDir);
//		logger.debug("dest dir = " + destDir);
		File xslDir = new File(XSL_DIR);
		FileFilter filter = new FileFilter() {
			@Override
			public boolean accept(File file) {
				return file.getName().toLowerCase().endsWith(".xsd");
			}
		};
		for (File xsdFile: sourceDir.listFiles(filter)) {
			String xsdFilename = xsdFile.getName();
			String xslFilename = xsdFilename.replaceAll("(.*)\\.[xX][sS][dD]", "$1.xslt");
			File xslFile = new File(xslDir,xslFilename);
			if (!xslFile.exists()) {
				xslFile = new File(xslDir, DEFAULT_XSL_FILE);
			}
			File transformedXsd = new File(destDir, xsdFilename);
//			logger.debug("XSL = " + xslFile);
//			logger.debug("XSD = " + xsdFile);
			XMLUtil.transform(xsdFile, xslFile, transformedXsd);
		}
	}
}
