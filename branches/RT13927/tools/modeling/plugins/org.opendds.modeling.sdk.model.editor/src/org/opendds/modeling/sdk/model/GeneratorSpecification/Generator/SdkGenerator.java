package org.opendds.modeling.sdk.model.GeneratorSpecification.Generator;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.net.MalformedURLException;
import java.net.URL;

import javax.xml.transform.Result;
import javax.xml.transform.Source;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerConfigurationException;
import javax.xml.transform.TransformerException;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.URIResolver;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;
import javax.xml.transform.stream.StreamSource;

import org.opendds.modeling.sdk.model.GeneratorSpecification.Generator.SdkGenerator.ErrorHandler.Severity;

import org.w3c.dom.Document;

public class SdkGenerator {

	private static TransformerFactory tFactory;
	private static Transformer idlTransformer;
	private static Transformer hTransformer;
	private static Transformer cppTransformer;
	private static Transformer mpcTransformer;
	
	private ParsedModelFile parsedModelFile;
	private ErrorHandler errorHandler;
	private FileProvider fileProvider;

	public static enum TransformType {
		IDL { public Transformer getTransformer() { return idlTransformer; }
		      public void setTransformer(Transformer t) { idlTransformer = t; }
	          public void transform(Source s, Result r) throws TransformerException { idlTransformer.transform(s, r); }
	          public String dialogTitle() { return "Generate IDL"; }
		      public String xslFilename() { return "xml/idl.xsl"; }
			  public String suffix() { return ".idl"; }
		    },
		H   { public Transformer getTransformer() { return hTransformer; }
	          public void setTransformer(Transformer t) { hTransformer = t; }
	          public void transform(Source s, Result r) throws TransformerException { hTransformer.transform(s, r); }
	          public String dialogTitle() { return "Generate C++ Header"; }
		      public String xslFilename() { return "xml/h.xsl"; }
			  public String suffix() { return "_T.h"; }
		    },
		CPP { public Transformer getTransformer() { return cppTransformer; }
	          public void setTransformer(Transformer t) { cppTransformer = t; }
	          public void transform(Source s, Result r) throws TransformerException { cppTransformer.transform(s, r); }
	          public String dialogTitle() { return "Generate C++ Body"; }
		      public String xslFilename() { return "xml/cpp.xsl"; }
			  public String suffix() { return "_T.cpp"; }
		    },
		MPC { public Transformer getTransformer() { return mpcTransformer; }
	          public void setTransformer(Transformer t) { mpcTransformer = t; }
		      public void transform(Source s, Result r) throws TransformerException { mpcTransformer.transform(s, r); }
		      public String dialogTitle() { return "Generate MPC"; }
		      public String xslFilename() { return "xml/mpc.xsl"; }
		  	  public String suffix() { return ".mpc"; }
		    };

		    public abstract Transformer getTransformer();
		    public abstract void setTransformer(Transformer t);
		    public abstract void transform(Source s, Result r) throws TransformerException;
			public abstract String dialogTitle();
		    public abstract String xslFilename();
			public abstract String suffix();
	};

	public static interface ErrorHandler {
		enum Severity {ERROR, WARNING, INFO};
		void error(Severity sev, String title, String message, Throwable exception);
	}
	
	public static interface FileProvider {
		URL fromWorkspace(String fileName) throws MalformedURLException;
		URL fromBundle(String fileName) throws MalformedURLException;
		void refresh(String targetFolder);
	}
	
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
	public static SdkGenerator create( FileProvider fp, ErrorHandler eh) {
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
	private SdkGenerator(FileProvider fp, ErrorHandler eh) {
		fileProvider = fp;
		errorHandler = eh;
		parsedModelFile = SdkGeneratorFactory.createParsedModelFile(fp, eh);
	}

	private TransformerFactory getTransformerFactory() {
		if (tFactory == null) {
			tFactory = TransformerFactory.newInstance();
		}
		return tFactory;
	}
	
	public void generate(TransformType which, String sourceName, String targetName) {
		boolean shouldReloadXSL = "true".equals(System.getProperty("reloadxsl"));
		if (shouldReloadXSL || which.getTransformer() == null) {
			try {
				URL xsl = fileProvider.fromBundle(which.xslFilename());
				TransformerFactory factory = getTransformerFactory();
				if (factory == null) {
					errorHandler.error(Severity.ERROR, which.dialogTitle(),
							"Unable to obtain a transformer factory.", null);
					return;
				}
				Source converter = new StreamSource(xsl.openStream());
				Transformer transformer = factory.newTransformer(converter);
				
				// TODO Base this on resources rather than string parsing.
				//      I suspect that if we phrase the names in terms of
				//      eclipse URI types (platform:/project/, platform:/plugin/,
				//      etc. that we may not need a 'special' one here.
				final String dir = which.xslFilename().substring(0, which.xslFilename().lastIndexOf("/"));
				final String sourceDir = sourceName.contains("/") ? sourceName.substring(0, sourceName.lastIndexOf("/")) : ".";
				transformer.setURIResolver(new URIResolver() {
					public Source resolve(String fname, String base) throws TransformerException {
						try {
							URL resource;
							if (fname.endsWith(".opendds")) {
								// This is a model reference, assume relative to the source file
								String file = sourceDir + File.separatorChar + fname;
								resource = fileProvider.fromWorkspace(file);
							} else {
								String file = fname.substring(0, 5).equals("file:")
								? fname.substring(5) : dir + File.separatorChar + fname;
								resource = fileProvider.fromBundle(file);
							}
							return new StreamSource(resource.openStream());
						} catch (IOException use) {
							throw new TransformerException("could not open " + fname);
						}
					}
				});
				which.setTransformer(transformer);

			} catch (TransformerConfigurationException e) {
				errorHandler.error(Severity.ERROR, which.dialogTitle(),
						"Failed to configure the transformer.", e);
			} catch (IOException e) {
				errorHandler.error(Severity.ERROR, which.dialogTitle(),
						"Failed opening XSL file " + which.xslFilename() + " for converter.", e);
			}
		}

		Result result = null;
		try {
			//ok to use getFile() instead of getFolder()?
			File targetFolder = new File(fileProvider.fromWorkspace(targetName).toURI());
			if (!targetFolder.exists()) {
				errorHandler.error(Severity.ERROR, which.dialogTitle(),
						"Target folder " + targetName + " does not exist.", null);
				return;
			}
	
			String modelname = parsedModelFile.getModelName(sourceName);
			if (modelname == null) {
				return; // messages were generated in the get call.
			}
	
			File output = new File(targetFolder, modelname + which.suffix());
			result = new StreamResult(new BufferedOutputStream(new FileOutputStream(output)));
		} catch (Exception e) {
			errorHandler.error(Severity.ERROR, which.dialogTitle(),
					"Unable to open the output file for conversion: " + result, e);
			return;
		}
		
		try {
			Document document = parsedModelFile.getModelDocument(sourceName);
			if (document == null) {
				return; // messages were generated in the get call.
			}
			Source source = new DOMSource(document);
			which.transform(source, result);

		} catch (TransformerException e) {
			errorHandler.error(Severity.ERROR, which.dialogTitle(),
					"Transformation failed!", e);
			return;
		}

		try {
			// Refresh the container where we just placed the result
			// as a service to the user.
			fileProvider.refresh(targetName);

		} catch (Exception e) {
			errorHandler.error(Severity.WARNING, which.dialogTitle(),
					"Unable to refresh output folder " + targetName, e);
		}
	}

	/// Allows testing of code generation outside of Eclipse, by instantiating
	/// the code generator with arguments that only use JDK types.
	public static void main(String[] args) {
		if (args.length < 1) {
			throw new IllegalArgumentException("Usage: CodeGenerator [-o targetDir] sourceFile");
		}
		File outputDir = new File(".");
		String inputFile = args[0];
		if (args.length > 2 && args[0].equals("-o")) {
			outputDir = new File(args[1]);
			if (!outputDir.exists()) outputDir.mkdir();
			inputFile = args[2];
		}
		
		SdkGenerator cg = SdkGeneratorFactory.createSdkGenerator( new FileProvider() {
			private final String bundle = System.getenv("DDS_ROOT")
				+ "/tools/modeling/plugins/org.opendds.modeling.sdk";
			@Override
			public void refresh(String targetFolder) {
			}
			@Override
			public URL fromWorkspace(String fileName) throws MalformedURLException {
				return new File(fileName).toURI().toURL();
			}
			@Override
			public URL fromBundle(String fileName) throws MalformedURLException {
				return new File(bundle, fileName).toURI().toURL();
			}
		}, new ErrorHandler() {
			@Override
			public void error(Severity sev, String title, String message, Throwable exception) {
				throw new RuntimeException(message, exception);
			}
		});

		for (TransformType tt : TransformType.values()) {
			cg.generate(tt, inputFile, outputDir.getPath());
		}
	}

}
