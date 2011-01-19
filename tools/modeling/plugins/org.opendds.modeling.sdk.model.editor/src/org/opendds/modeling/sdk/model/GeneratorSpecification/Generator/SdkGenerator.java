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
import javax.xml.transform.dom.DOMResult;
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
	private static Transformer mpbTransformer;
	private static Transformer traitsHTransformer;
	private static Transformer traitsCppTransformer;
	private static Transformer preprocTransformer;

	private ParsedModelFile parsedModelFile;
	private ParsedGeneratorFile parsedGeneratorFile;
	private ErrorHandler errorHandler;
	private FileProvider fileProvider;

	public static enum TransformType {
		IDL { public Transformer getTransformer() { return idlTransformer; }
		      public void setTransformer(Transformer t) { idlTransformer = t; }
	          public void transform(Source s, Result r) throws TransformerException { idlTransformer.transform(s, r); }
	          public String dialogTitle() { return "Generate IDL"; }
		      public String xslFilename() { return "xml/idl.xsl"; }
			  public String suffix() { return ".idl"; }
			  public boolean usesPreproc() { return true; }
		    },
		H   { public Transformer getTransformer() { return hTransformer; }
	          public void setTransformer(Transformer t) { hTransformer = t; }
	          public void transform(Source s, Result r) throws TransformerException { hTransformer.transform(s, r); }
	          public String dialogTitle() { return "Generate C++ Header"; }
		      public String xslFilename() { return "xml/h.xsl"; }
			  public String suffix() { return "_T.h"; }
			  public boolean usesPreproc() { return true; }
		    },
		CPP { public Transformer getTransformer() { return cppTransformer; }
	          public void setTransformer(Transformer t) { cppTransformer = t; }
	          public void transform(Source s, Result r) throws TransformerException { cppTransformer.transform(s, r); }
	          public String dialogTitle() { return "Generate C++ Body"; }
		      public String xslFilename() { return "xml/cpp.xsl"; }
			  public String suffix() { return "_T.cpp"; }
			  public boolean usesPreproc() { return true; }
		    },
		MPC { public Transformer getTransformer() { return mpcTransformer; }
	          public void setTransformer(Transformer t) { mpcTransformer = t; }
		      public void transform(Source s, Result r) throws TransformerException { mpcTransformer.transform(s, r); }
		      public String dialogTitle() { return "Generate MPC"; }
		      public String xslFilename() { return "xml/mpc.xsl"; }
		  	  public String suffix() { return ".mpc"; }
			  public boolean usesPreproc() { return false; }
		    },
		MPB { public Transformer getTransformer() { return mpbTransformer; }
	          public void setTransformer(Transformer t) { mpbTransformer = t; }
		      public void transform(Source s, Result r) throws TransformerException { mpbTransformer.transform(s, r); }
		      public String dialogTitle() { return "Generate MPB"; }
		      public String xslFilename() { return "xml/mpb.xsl"; }
		  	  public String suffix() { return ".mpb"; }
			  public boolean usesPreproc() { return false; }
		    },
		TRH { public Transformer getTransformer() { return traitsHTransformer; }
	          public void setTransformer(Transformer t) { traitsHTransformer = t; }
		      public void transform(Source s, Result r) throws TransformerException { traitsHTransformer.transform(s, r); }
		      public String dialogTitle() { return "Generate Traits Header"; }
		      public String xslFilename() { return "xml/traits_h.xsl"; }
		  	  public String suffix() { return "Traits.h"; }
			  public boolean usesPreproc() { return false; }
		    },
		TRC { public Transformer getTransformer() { return traitsCppTransformer; }
	          public void setTransformer(Transformer t) { traitsCppTransformer = t; }
		      public void transform(Source s, Result r) throws TransformerException { traitsCppTransformer.transform(s, r); }
		      public String dialogTitle() { return "Generate Traits Body"; }
		      public String xslFilename() { return "xml/traits_cpp.xsl"; }
		  	  public String suffix() { return "Traits.cpp"; }
			  public boolean usesPreproc() { return false; }
		    };

		    public abstract Transformer getTransformer();
		    public abstract void setTransformer(Transformer t);
		    public abstract void transform(Source s, Result r) throws TransformerException;
			public abstract String dialogTitle();
		    public abstract String xslFilename();
			public abstract String suffix();
			public abstract boolean usesPreproc();
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
		parsedModelFile = ParsedModelFile.create(fp, eh);
		parsedGeneratorFile = ParsedGeneratorFile.create(fp, eh);
	}

	private TransformerFactory getTransformerFactory() {
		if (tFactory == null) {
			tFactory = TransformerFactory.newInstance();
		}
		return tFactory;
	}
	
	public void generate(TransformType which, String sourceName) {
		final String dir = which.xslFilename().substring(0, which.xslFilename().lastIndexOf("/"));
		final String sourceDir = sourceName.contains("/") ? sourceName.substring(0, sourceName.lastIndexOf("/")) : ".";
		URIResolver resolver = new URIResolver() {
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
		};

		boolean shouldReloadXSL = "true".equals(System.getProperty("reloadxsl"));
		boolean usesPreproc = which.usesPreproc();
		if (shouldReloadXSL || which.getTransformer() == null) {
			String currentFile = which.xslFilename();
			try {
				URL xsl = fileProvider.fromBundle(currentFile);
				TransformerFactory factory = getTransformerFactory();
				if (factory == null) {
					errorHandler.error(Severity.ERROR, which.dialogTitle(),
							"Unable to obtain a transformer factory.", null);
					return;
				}
				factory.setURIResolver(resolver);
				Source converter = new StreamSource(xsl.openStream());
				Transformer transformer = factory.newTransformer(converter);
				
				which.setTransformer(transformer);

				if (usesPreproc && (preprocTransformer == null || shouldReloadXSL)) {
					currentFile = "xml/preprocess.xsl";
					xsl = fileProvider.fromBundle(currentFile);
					preprocTransformer = factory.newTransformer(new StreamSource(xsl.openStream()));
				}
			} catch (TransformerConfigurationException e) {
				errorHandler.error(Severity.ERROR, which.dialogTitle(),
						"Failed to configure the transformer.", e);
				return;
			} catch (IOException e) {
				errorHandler.error(Severity.ERROR, which.dialogTitle(),
						"Failed opening XSL file " + currentFile + " for converter.", e);
				return;
			}
		}

		which.getTransformer().setURIResolver(resolver);
		if (usesPreproc) preprocTransformer.setURIResolver(resolver);

		String openddsFile = parsedGeneratorFile.getSource(sourceName);
		String targetDir = parsedGeneratorFile.getTarget(/* use previous sourceName */);
		if (openddsFile == null || targetDir == null) {
			return; // messages were generated in the get call.
		}
		
		Result result = null;
		try {
			File targetFolder = new File(fileProvider.fromWorkspace(targetDir).toURI());
			if (!targetFolder.exists()) {
				targetFolder.mkdirs();
			}

			String modelname = parsedModelFile.getModelName(openddsFile);
			if (modelname == null) {
				return; // messages were generated in the get call.
			}
			
			if (which == TransformType.MPB) {
				Boolean dataLib = parsedModelFile.hasDataLib();
				if (dataLib != null && !dataLib.booleanValue()) {
					return; // don't generate mpb if there's no dataLib
				}
			}

			File output = new File(targetFolder, modelname + which.suffix());
			result = new StreamResult(new BufferedOutputStream(new FileOutputStream(output)));
		} catch (Exception e) {
			errorHandler.error(Severity.ERROR, which.dialogTitle(),
					"Unable to open the output file for conversion: " + result, e);
			return;
		}
		
		try {
			Source source;
			
			if (usesPreproc) {
				Document document = parsedModelFile.getModelDocument();
				if (document == null) {
					return; // messages were generated in the get call.
				}
				DOMResult temp = new DOMResult();
				preprocTransformer.transform(new DOMSource(document), temp);
				source = new DOMSource(temp.getNode());
			} else {
				Document document = parsedGeneratorFile.getModelDocument();
				if (document == null) {
					return; // messages were generated in the get call.
				}
				source = new DOMSource(document);
			}
			
			which.transform(source, result);

		} catch (TransformerException e) {
			errorHandler.error(Severity.ERROR, which.dialogTitle(),
					"Transformation failed!", e);
			return;
		}

		try {
			// Refresh the container where we just placed the result
			// as a service to the user.
			fileProvider.refresh(targetDir);

		} catch (Exception e) {
			errorHandler.error(Severity.WARNING, which.dialogTitle(),
					"Unable to refresh output folder " + targetDir, e);
		}
	}

	/// Allows running code generation outside of Eclipse, by instantiating
	/// the code generator with arguments that only use JDK types.
	public static void main(String[] args) {
		if (args.length < 1) {
			throw new IllegalArgumentException("Usage: CodeGenerator sourceFile");
		}
		String inputFile = args[0];
		SdkGenerator cg = SdkGenerator.create( new FileProvider() {
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
			cg.generate(tt, inputFile);
		}
	}

}
