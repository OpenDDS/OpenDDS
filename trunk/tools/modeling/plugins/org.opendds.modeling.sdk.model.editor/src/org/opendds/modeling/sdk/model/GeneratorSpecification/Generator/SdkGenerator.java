package org.opendds.modeling.sdk.model.GeneratorSpecification.Generator;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.net.MalformedURLException;
import java.net.URISyntaxException;
import java.net.URL;

import javax.xml.transform.Result;
import javax.xml.transform.Source;
import javax.xml.transform.TransformerException;
import javax.xml.transform.URIResolver;
import javax.xml.transform.stream.StreamResult;
import javax.xml.transform.stream.StreamSource;
import javax.xml.validation.Schema;
import javax.xml.validation.SchemaFactory;
import javax.xml.validation.Validator;

import org.xml.sax.SAXException;

public class SdkGenerator {

	protected IErrorHandler errorHandler;
	protected IFileProvider fileProvider;
	protected SdkTransformer transformer;
	protected ParsedGeneratorFile parsedGeneratorFile;
	private long newestXslTimestamp;
		
	/**
	 * Create a new instance of the SDK code generation class.
	 * 
	 * This is the correct way to obtain new instances of the SDK code
	 * generation class.
	 * 
	 * @param fp - file provider to be used by the SDK code generation class.
	 * @param eh - error handler to be used by the SDK code generation class.
	 * @return   - a new instance of the SDK code generation class.
	 */
	public static SdkGenerator create( IFileProvider fp, IErrorHandler eh) {
		return new SdkGenerator( fp, eh);
	}

	/**
	 * Construct an instance of the SDK code generation class.
	 * 
	 * The correct way to create new instances of the SDK code generator
	 * is to call the static create() method.
	 * 
	 * @param fp - file provider for the code generator.
	 * @param eh - error handler for the code generator.
	 */
	private SdkGenerator(IFileProvider fp, IErrorHandler eh) {
		fileProvider = fp;
		errorHandler = eh;
		parsedGeneratorFile = ParsedGeneratorFile.create(fp, eh);
		transformer = SdkTransformer.create(fp, eh);
		transformer.setResolver( getResolverFor(null));
	}

	//
	// Delegate most of the interface implementations to the contained generator model.
	//
	
	public void setGeneratorModel( IGeneratorModel generatorModel) {
		parsedGeneratorFile.setGeneratorModel(generatorModel);
	}
	
	public void setEditingDomain( Object editingDomain) {
		parsedGeneratorFile.setEditingDomain(editingDomain);
	}
	
	public boolean isModelSource( Object element) {
		return parsedGeneratorFile.isModelSource( element);
	}
	
	public boolean isModelTarget( Object element) {
		return parsedGeneratorFile.isModelTarget( element);
	}

	/**
	 * TODO Determine how to handle setting the source name here when an EMF
	 *      model is implemented.  That should never happen, so defer resolution
	 *      for now.
	 *      
	 * @param sourceName name of the file containing the code generator specification.
	 */
	public void setSourceName( String sourceName) {
		parsedGeneratorFile.setSourceName(sourceName);
		transformer.setResolver( getResolverFor(sourceName));
	}
	
	public String getModelFileName() {
		return parsedGeneratorFile.getModelFileName();
	}
	
	public void setModelFileName( String modelFileName) {
		parsedGeneratorFile.setModelFileName(modelFileName);
	}
	
	public String getTargetDirName() {
		return parsedGeneratorFile.getTargetDirName();
	}
	
	public void setTargetDirName( String targetDirName) {
		parsedGeneratorFile.setTargetDirName(targetDirName);
	}
	
	public String getModelName() {
		return parsedGeneratorFile.getModelName();
	}
	
	public void generateAll() {
		generate(SdkTransformer.TransformType.IDL);
		generate(SdkTransformer.TransformType.H);
		generate(SdkTransformer.TransformType.CPP);
		generate(SdkTransformer.TransformType.TRH);
		generate(SdkTransformer.TransformType.TRC);
		generate(SdkTransformer.TransformType.MPC);
		generate(SdkTransformer.TransformType.MPB);
		generate(SdkTransformer.TransformType.PATH_MPB);
	}
	
	/**
	 * Generate the specified output code using the contained generator model.
	 * 
	 * @param which the type of code to generate.
	 */
	public void generate(SdkTransformer.TransformType which) {
		String modelFileName = getModelFileName();
		if( modelFileName == null || modelFileName.isEmpty()) {
			return; // messages were generated in the get call.
		}

		String targetDirName = getTargetDirName();
		if( targetDirName == null || targetDirName.isEmpty()) {
			return; // messages were generated in the get call.
		}

		Result result = null;
		BufferedOutputStream bos = null;
		try {
			URL targetUrl = fileProvider.fromWorkspace(targetDirName, true);
			if( targetUrl == null) {
				return;
			}

			File targetFolder = new File(targetUrl.toURI());
			if (!targetFolder.exists()) {
				targetFolder.mkdirs();
			}

			String modelname = getModelName();
			if (modelname == null) {
				errorHandler.error(IErrorHandler.Severity.ERROR, which.getText(),
						"Model name not specified", null);
				return;
			}

			File output = new File( targetFolder, modelname + which.getSuffix());

			if (output.exists() && !outOfDate(output)) {
				return; // no need to re-generate
			}

			result = new StreamResult(bos = new BufferedOutputStream(new FileOutputStream(output)));

		} catch (Exception e) {
			errorHandler.error(IErrorHandler.Severity.ERROR, which.getText(),
					"Unable to open the output file for conversion: " + result, e);
			return;
		}

		Source source = parsedGeneratorFile.getSource( which.needsResolvedModel()? transformer: null);
		if( source != null) {
			transformer.transform(which, source, result);
			try {
				bos.close();
			} catch (IOException ioe) {
				errorHandler.error(IErrorHandler.Severity.ERROR, ioe.getMessage(),
						"Unable to close the output file: " , ioe);
				return;				
			}
		}
		
		try {
			// Refresh the container where we just placed the result
			// as a service to the user.
			fileProvider.refresh(targetDirName);

		} catch (Exception e) {
			errorHandler.error(IErrorHandler.Severity.WARNING, which.getText(),
					"Unable to refresh output folder " + getTargetDirName(), e);
		}
	}

	@SuppressWarnings("unused")
	private boolean isXSDValid(String filename) {
        // 1. Lookup a factory for the W3C XML Schema language
        SchemaFactory factory = 
            SchemaFactory.newInstance("http://www.w3.org/2001/XMLSchema");
        
        // 2. Compile the schema. 
        // Here the schema is loaded from a java.io.File, but you could use 
        // a java.net.URL or a javax.xml.transform.Source instead.
        File schemaLocation = new File("../../plugins/org.opendds.modeling.model/model/OpenDDSXMI.xsd");
        try {
          Schema schema = factory.newSchema(schemaLocation);
          // 3. Get a validator from the schema.
          Validator validator = schema.newValidator();
          
          // 4. Parse the document you want to check.
          Source source = new StreamSource(filename);
          
          // 5. Check the document
          try {
              validator.validate(source);
              System.out.println(filename + " is valid.");
          }
          catch (IOException ioe) {
              System.out.println(filename + " could not be loaded.");
        	  
          }
          catch (SAXException ex) {
              System.out.println(filename + " is not valid because ");
              System.out.println(ex.getMessage());
          }  

        } catch (SAXException se) {
        	System.out.println("Schema SAXException");
            System.out.println(se.getMessage());
        	return false;
        }
        return true;
	}

	/// is the output file out-of-date with respect to the inputs?
	private boolean outOfDate(File output) {
		long genfileTimestamp = parsedGeneratorFile.getTimestamp(),
			modelTimestamp = parsedGeneratorFile.parsedModelFile.getTimestamp();
		if (genfileTimestamp == 0 || modelTimestamp == 0) {
			return true; // assume out-of-date if either timestamp is unknown
		}
		long outputTimestamp = output.lastModified();
		return (outputTimestamp < genfileTimestamp)
			|| (outputTimestamp < modelTimestamp)
			|| xslOutOfDate(outputTimestamp);
	}

	private boolean xslOutOfDate(long outputTimestamp) {
		String prop = System.getProperty("opendds.checkXslTimestamps");
		if (prop == null || "false".equalsIgnoreCase(prop)) {
			return false;
		}

		if (newestXslTimestamp == 0) { // only need to compute this once, xsl won't change in-session
			try {
				for (SdkTransformer.TransformType tt : SdkTransformer.TransformType.values()) {
					URL xsl = fileProvider.fromBundle(tt.xslFilename());
					long time = new File(xsl.toURI()).lastModified();
					if (time > newestXslTimestamp) {
						newestXslTimestamp = time;
					}
				}
				for (String other : SdkTransformer.UTIL_FILES) {
					URL xsl = fileProvider.fromBundle(other);
					long time = new File(xsl.toURI()).lastModified();
					if (time > newestXslTimestamp) {
						newestXslTimestamp = time;
					}					
				}
			} catch (MalformedURLException e) {
				return true; // assume it's out of date if we can't get the xsl
			} catch (URISyntaxException e) {
				return true; // assume it's out of date if we can't get the xsl
			}
		}
		return outputTimestamp < newestXslTimestamp;
	}

	protected URIResolver getResolverFor( String path) {
		final String start = path != null && path.contains("/") ?
				path.substring(0, path.lastIndexOf("/") + 1) :
				"";

		return new URIResolver() {
			public Source resolve(String fname, String base) throws TransformerException {
				try {
					URL resource;
					if (fname.endsWith(".xsl") || fname.endsWith(".xml")) {
						// Force XSL and XML files to be located in the 'xsl' subdirectory.
						String file = fname.substring(0, 5).equals("file:")
							? fname.substring(5) : "xsl" + File.separatorChar + fname;
						resource = fileProvider.fromBundle(file);
						
					} else {
						// This is a model reference, assume relative to the source file
						resource = fileProvider.fromWorkspace(start + fname);
						if (resource == null) {
							throw new IOException();
						}
					}
					return new StreamSource(resource.openStream());

				} catch (IOException use) {
					throw new TransformerException("Custom resolver with start " + start + " could not open " + fname);
				}
			}
		};
	}

	/// Allows running code generation outside of Eclipse, by instantiating
	/// the code generator with arguments that only use JDK types.
	public static void main(String[] args) {
		if (args.length < 1) {
			throw new IllegalArgumentException("Usage: CodeGenerator sourceFile");
		}
		String inputFile = args[0];
		SdkGenerator cg = SdkGenerator.create( new IFileProvider() {
			private final String bundle = System.getenv("DDS_ROOT")
				+ "/tools/modeling/plugins/org.opendds.modeling.sdk.model";
			@Override
			public void refresh(String targetFolder) {
			}
			@Override
			public URL fromWorkspace(String fileName) throws MalformedURLException {
				return fromWorkspace(fileName, false);
			}
			@Override
			public URL fromWorkspace(String fileName, boolean directory) throws MalformedURLException {
				return new File(fileName).toURI().toURL();
			}
			@Override
			public URL fromBundle(String fileName) throws MalformedURLException {
				return new File(bundle, fileName).toURI().toURL();
			}
		}, new IErrorHandler() {
			@Override
			public void error(Severity sev, String title, String message, Throwable exception) {
				throw new RuntimeException(message, exception);
			}
		});

		cg.setSourceName(inputFile);
		cg.generateAll();
	}

}
