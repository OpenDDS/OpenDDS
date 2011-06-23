package org.opendds.modeling.validation;

import java.io.File;
import java.util.List;

import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.CommandLineParser;
import org.apache.commons.cli.HelpFormatter;
import org.apache.commons.cli.Option;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;
import org.apache.commons.cli.PosixParser;
import org.apache.log4j.Logger;
import org.xml.sax.SAXParseException;

import com.ociweb.xml.util.XMLUtil;

public class OpenDDSModelValidator {
	private static final Logger logger = Logger.getLogger(OpenDDSModelValidator.class.getName());

	static final String XSD_FILE_PROPERTY = "opendds.xsd.file";
	static final String XSD_DEFAULT_FILE = "xsd/OpenDDSXMI.xsd";
	static final String APP_NAME_PROPERTY = "app.name";
	static final String FILE_ARG = "file";
	static final String DIR_ARG = "dir";
	static final String RECURSIVE_ARG = "recursive";
	
	public static void main(String[] args) {
		
		CommandLineParser parser = new PosixParser();
		Options options = new Options();
		Option fileOpt = new Option("f", FILE_ARG, true, "opendds file(s) to validate");
		fileOpt.setArgs(Integer.MAX_VALUE);
		options.addOption(fileOpt);
		options.addOption("d", DIR_ARG, true, "directory of opendds files to validate");
		options.addOption("r", RECURSIVE_ARG, false, "search directory recursively for opendds files");
		
		try {
			String xsdFilename = System.getProperty(XSD_FILE_PROPERTY, XSD_DEFAULT_FILE);				
			CommandLine line = parser.parse(options, args);
			
			if (!(line.hasOption(FILE_ARG) || line.hasOption(DIR_ARG))) {
				HelpFormatter formatter = new HelpFormatter();
				formatter.printHelp(
						System.getProperty(APP_NAME_PROPERTY, OpenDDSModelValidator.class.getSimpleName()), 
						options);
			}
			String[] files = line.getOptionValues(FILE_ARG);
			String dir = line.getOptionValue(DIR_ARG);
			boolean recursive = line.hasOption(RECURSIVE_ARG);
			if (files != null) {
				validateFiles(new File(xsdFilename), files);
			}
			if (dir != null) {
				validateDir(new File(dir), recursive);
			}
		}
		catch( ParseException exp ) {
			logger.error("Encountered an unexpected error:" + exp.getMessage());
		}
	}
	
	static void validateFile(final File xsd, final File xml) {
		try {
			List<SAXParseException> errors = XMLUtil.validate(xsd, xml);
			if (!errors.isEmpty()) {
				logger.error("Error: while validating " + xml + " the following errors were found:");
				for (SAXParseException se : errors) {
					logger.error("    Line " + se.getLineNumber() + " Column " + se.getColumnNumber() + ": " + se.getMessage());
				}
			} else {
				logger.info("Validated " + xml);
			}
		} catch (Exception e) {
			logger.error("Fatal validation Error: " + e.getMessage());
		}
	}
	
	static void validateFiles(final File xsd, final String[] files) {
		for (String filename : files) {
			validateFile(xsd, new File(filename));
		}
	}
	
	static void validateDir(final File parent, final boolean recursive){
		File xsdFile = new File(System.getProperty(XSD_FILE_PROPERTY, XSD_DEFAULT_FILE));				
		for (File f: parent.listFiles()) {
			logger.debug("Checking " + f.getPath());
			if (f.isDirectory() && recursive) {
				validateDir(f, true);
			} else if (f.getName().toLowerCase().endsWith(".opendds")) {
				validateFile(xsdFile, f);
			}
		}
	}
}
