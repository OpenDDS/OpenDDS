package org.opendds.modeling.validation;

import java.io.File;
import java.io.FileFilter;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.CommandLineParser;
import org.apache.commons.cli.HelpFormatter;
import org.apache.commons.cli.Option;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;
import org.apache.commons.cli.PosixParser;
import org.xml.sax.SAXParseException;

import com.ociweb.xml.util.XMLUtil;

// TODO utilize eclipse built-in logging framework
public class OpenDDSModelValidator {

	static final String OPENDDS_XSD_FILE_PROPERTY = "opendds.xsd.file";
	static final String OPENDDS_FILE_EXT = ".opendds";
	static final String DEFAULT_OPENDDS_XSD = "xsd/OpenDDSXMI.xsd";

	static final String CODEGEN_XSD_FILE_PROPERTY = "codegen.xsd.file";
	static final String CODEGEN_FILE_EXT = ".codegen";
	static final String DEFAULT_CODEGEN_XSD = "xsd/GeneratorXMI.xsd";

	static final String APP_NAME_PROPERTY = "app.name";
	static final String FILE_ARG = "file";
	static final String RECURSIVE_ARG = "recursive";
	static final String QUIET_ARG = "quiet";

	private final Map<String,String> schemas = new HashMap<String,String>();

	private final List<File> files;
	private final boolean recursive;
	private final boolean quiet;

	public OpenDDSModelValidator(boolean recursive, boolean quiet, List<File> files) {
	    this.files = files;
	    this.recursive = recursive;
	    this.quiet = quiet;

	    String openddsXsd = System.getProperty(OPENDDS_XSD_FILE_PROPERTY, DEFAULT_OPENDDS_XSD);
	    schemas.put(OPENDDS_FILE_EXT, openddsXsd);
	    String codegenXsd = System.getProperty(CODEGEN_XSD_FILE_PROPERTY, DEFAULT_CODEGEN_XSD);
	    schemas.put(CODEGEN_FILE_EXT, codegenXsd);
	}

	public static void main(String[] args) {
		CommandLineParser parser = new PosixParser();
		Options options = new Options();
		Option fileOpt = new Option("f", FILE_ARG, true, "opendds file(s) and/or directory(s) to validate");
		fileOpt.setArgs(Integer.MAX_VALUE);
		options.addOption(fileOpt);
		options.addOption("r", RECURSIVE_ARG, false, "search directory recursively for opendds files");
		options.addOption("q", QUIET_ARG, false, "suppress output except for error reporting");

		try {
			CommandLine line = parser.parse(options, args, false);
			if (!line.hasOption(FILE_ARG)) {
				HelpFormatter formatter = new HelpFormatter();
				formatter.printHelp(
						System.getProperty(APP_NAME_PROPERTY, OpenDDSModelValidator.class.getSimpleName()),
						options);
				System.exit(-1);
			}

			String[] filenames = line.getOptionValues(FILE_ARG);
			boolean recursive = line.hasOption(RECURSIVE_ARG);
			boolean quiet = line.hasOption(QUIET_ARG);
			List<File> files = new ArrayList<File>();
			for (String filename : filenames) {
			    files.add(new File(filename));
            }
			OpenDDSModelValidator validator = new OpenDDSModelValidator(recursive, quiet, files);
			int numErrs = validator.validate();
			System.exit(numErrs);
		} catch (ParseException exp) {
			System.err.println("Encountered an unexpected error:" + exp.getMessage());
		}
	}

    public int validate() {
        if (quiet) {
            System.out.println("Starting validation in quiet mode (error reporting only)...");
        } else {
            System.out.println("Starting validation...");
        }
        int numInvalidFiles = 0;
        for (File file : files) {
            if (!file.exists()) {
            	System.err.println(file + " does not exist!");
                continue;
            }
//            logger.debug("Checking file: " + file);
            if (file.isDirectory()) {
                numInvalidFiles += validateDir(file);
            } else {
                numInvalidFiles += validateFile(file) ? 0 : 1;
            }
        }
        if (numInvalidFiles == 0) {
        	System.out.println("Validation completed successfully.");
		} else {
			System.err.println("Validation completed with errors in " + numInvalidFiles + " files.");
		}
        return numInvalidFiles;
    }

	private boolean validateFile(File xml) {
	    boolean valid = false;
		try {
//		    logger.debug("xml = " + xml);
		    String ext = xml.getName().replaceAll(".+(\\..+)", "$1");
		    String xsdFile = schemas.get(ext);
//		    logger.debug("xsdFile = " + xsdFile);
		    File xsd = new File(xsdFile);
		    if (!xsd.exists()) {
//		        logger.debug("working dir = " + System.getProperty("user.dir"));
                throw new IllegalStateException(xsd + " does not exist");
            }
			List<SAXParseException> errors = XMLUtil.validate(xsd, xml);
			if (!errors.isEmpty()) {
				System.err.println("ERROR: while validating " + xml + " the following errors were found:");
				for (SAXParseException se : errors) {
					System.err.println("    Line " + se.getLineNumber() + " Column " +
					        se.getColumnNumber() + ": " + se.getMessage());
				}
			} else {
			    if (!quiet) {
			        System.out.println("Validated " + xml);
			    }
			    valid = true;
			}
			return valid;
		} catch (RuntimeException e) {
			System.err.println("Unexpected error");
			e.printStackTrace();
			throw e;
		} catch (Exception e) {
		    System.err.println("Fatal validation Error: " + e.getMessage());
		    return false;
		}
	}

	private int validateDir(File parent){
	    int numInvalidFiles = 0;
        FileFilter filter = new FileFilter() {
            @Override
            public boolean accept(File file) {
                if (file.isDirectory()) {
                    return true;
                }
                String ext = file.getName().replaceAll(".+(\\..+)", "$1");
                return schemas.containsKey(ext);
            }
        };
		for (File f: parent.listFiles(filter)) {
//			logger.debug("Checking " + f.getPath());
			if (f.isDirectory()) {
				if (!recursive) {
					continue;
				}
			    numInvalidFiles += validateDir(f);
			} else {
			    numInvalidFiles += validateFile(f) ? 0 : 1;
			}
		}
		return numInvalidFiles;
	}
}
