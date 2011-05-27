package org.opendds.modeling.sdk.model.GeneratorSpecification.Generator;

import java.io.IOException;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.HashMap;
import java.util.Map;

import javax.xml.transform.Result;
import javax.xml.transform.Source;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerException;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.URIResolver;
import javax.xml.transform.stream.StreamSource;

public class SdkTransformer {

	private static TransformerFactory tFactory;
	private static Map<TransformType, Transformer> transformers = new HashMap<TransformType, Transformer>();

	private IErrorHandler errorHandler;
	private IFileProvider fileProvider;
	private URIResolver resolver;

	// files that are utilities used by the various xslFilename() files
	static final String[] UTIL_FILES = {"xsl/lut.xml", "xsl/common.xsl"};

	public static enum TransformType {
		IDL { public String getText() { return "Generate IDL"; }
			  public String xslFilename() { return "xsl/idl.xsl"; }
			  public String getSuffix() { return ".idl"; }
		      public boolean needsResolvedModel() { return true; }
		},
		H   { public String getText() { return "Generate C++ Header"; }
		      public String xslFilename() { return "xsl/h.xsl"; }
		      public String getSuffix() { return "_T.h"; }
		      public boolean needsResolvedModel() { return true; }
		},
		CPP { public String getText() { return "Generate C++ Body"; }
	          public String xslFilename() { return "xsl/cpp.xsl"; }
	          public String getSuffix() { return "_T.cpp"; }
		      public boolean needsResolvedModel() { return true; }
		},
		MPC { public String getText() { return "Generate MPC"; }
	          public String xslFilename() { return "xsl/mpc.xsl"; }
	          public String getSuffix() { return ".mpc"; }
		      public boolean needsResolvedModel() { return false; }
		},
		MPB { public String getText() { return "Generate MPB"; }
	          public String xslFilename() { return "xsl/mpb.xsl"; }
	          public String getSuffix() { return ".mpb"; }
		      public boolean needsResolvedModel() { return false; }
		},
		PATH_MPB { public String getText() { return "Generate Paths MPB"; }
		      public String xslFilename() { return "xsl/paths_mpb.xsl"; }
		      public String getSuffix() { return "_paths.mpb"; }
		      public boolean needsResolvedModel() { return false; }
		},
		TRH { public String getText() { return "Generate Traits Header"; }
	          public String xslFilename() { return "xsl/traits_h.xsl"; }
	          public String getSuffix() { return "Traits.h"; }
		      public boolean needsResolvedModel() { return false; }
		},
		TRC { public String getText() { return "Generate Traits Body"; }
		      public String xslFilename() { return "xsl/traits_cpp.xsl"; }
		      public String getSuffix() { return "Traits.cpp"; }
		      public boolean needsResolvedModel() { return false; }
		},
		RESOLVED { 
		      public String getText() { return "Preprocessor to resolve external references"; }
		      public String xslFilename() { return "xsl/preprocess.xsl"; }
		      public String getSuffix() { return ".openddsx"; }
		      public boolean needsResolvedModel() { return false; }
		};

		public abstract String getText();
		public abstract String xslFilename();
		public abstract String getSuffix();
		public abstract boolean needsResolvedModel();
	};
	
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
	public static SdkTransformer create( IFileProvider fp, IErrorHandler eh) {
		return new SdkTransformer( fp, eh);
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
	private SdkTransformer(IFileProvider fp, IErrorHandler eh) {
		fileProvider = fp;
		errorHandler = eh;
	}

	private TransformerFactory getTransformerFactory() {
		if (tFactory == null) {
			tFactory = TransformerFactory.newInstance();
		}
		return tFactory;
	}
	
	private Transformer getTransformer( TransformType which) {
		Transformer transformer = transformers.get(which);
		if( transformer == null) {
			TransformerFactory factory = getTransformerFactory();
			if (factory == null) {
				errorHandler.error(IErrorHandler.Severity.ERROR, which.getText(),
						"Unable to obtain a transformer factory.", null);
				return null;
			}
			if( resolver != null) {
				factory.setURIResolver(resolver);
			}

			try {
				URL xsl = fileProvider.fromBundle(which.xslFilename());
				Source converter = new StreamSource(xsl.openStream());
				transformer = factory.newTransformer(converter);

			} catch (MalformedURLException e) {
				errorHandler.error(IErrorHandler.Severity.ERROR, which.getText(),
						"Failed to locate the transformer XSLT source. ", e);
				return null;
				
			} catch (IOException e) {
				errorHandler.error(IErrorHandler.Severity.ERROR, which.getText(),
						"Failed to read the transformer XSLT source. ", e);
				return null;
				
			} catch (TransformerException e) {
				errorHandler.error(IErrorHandler.Severity.ERROR, which.getText(),
						"Failed to create the transformer. ", e);
				return null;
			}
			
			transformers.put(which, transformer);
		}
		return transformer;
	}
	
	/**
	 * Set the resolver to use for transformations.
	 * 
	 * The resolver should be reset when the path to the input file
	 * (or XSL file) is changed from the last one.  This path is used
	 * to search for files referenced within the XSL stylesheet.
	 * 
	 * @param resolver implementation of the resolver to use for transformations.
	 */
	public void setResolver( URIResolver resolver) {
		if( resolver != null) {
			this.resolver = resolver;
		}
	}
	
	/**
	 * Transform node-sets.
	 * 
	 * Delegate for transformations of any type.  This will transform the
	 * Source node-set into the Result node-set using the (possibly cached)
	 * transformer for the type of transformation indicated by 'which'.
	 * 
	 * @param which which transformation to perform.
	 * @param source source node-set to be transformed.
	 * @param result destination node-set containing the transformation results.
	 */
	public void transform(TransformType which, Source source, Result result) {
		Transformer transformer = getTransformer(which);
		if( transformer == null) {
			return; // messages were generated in the get call.
		}
		
		try {
			// Set the resolver each time we transform to be sure its
			// the right one.
			if( resolver != null) {
				transformer.setURIResolver(resolver);
			}
			transformer.transform(source, result);

		} catch (TransformerException e) {
			errorHandler.error(IErrorHandler.Severity.ERROR, which.getText(),
					"Transformation failed!", e);
			return;
		}
	}

}
