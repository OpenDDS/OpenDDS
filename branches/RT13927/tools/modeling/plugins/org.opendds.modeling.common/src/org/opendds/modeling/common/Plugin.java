package org.opendds.modeling.common;

import java.util.HashMap;
import java.util.Map;

import org.eclipse.emf.common.EMFPlugin;
import org.eclipse.emf.common.util.ResourceLocator;

/**
 * The activator class controls the plug-in life cycle
 */
public class Plugin extends EMFPlugin {

	/**
	 *  The plug-in ID
	 */
	public static final String PLUGIN_ID = "org.opendds.modeling.common";

	/**
	 * The plug-in singleton.
	 */
	public static final Plugin INSTANCE = new Plugin();
	
	/**
	 * Actual image names.
	 */
	protected static final String DEFAULT_IMAGE_NAME         = "DefaultImage";
	protected static final String BOOLEAN_IMAGE_NAME         = "boolean";
	protected static final String INSTANCE_IMAGE_NAME        = "instance";
	protected static final String NODE_IMAGE_NAME            = "node";
	protected static final String NUMBER_IMAGE_NAME          = "number";
	protected static final String PACKAGE_IMAGE_NAME         = "package";
	protected static final String STRING_IMAGE_NAME          = "string";
	protected static final String GENERATOR_IMAGE_NAME       = "generator";
	protected static final String GENERATOR_LARGE_IMAGE_NAME = "generator-large";

	/**
	 * Image mappings.
	 */
	protected static final Map<String,String> imageMappings = new HashMap<String,String>();
	
	/**
	 * Initialize the item to image mappings.
	 */
	static {
		// org.opendds.modeling.sdk.model.edit
		imageMappings.put("CodeGen",                    NODE_IMAGE_NAME);
		imageMappings.put("ConnRetryAttempts",          NUMBER_IMAGE_NAME);
		imageMappings.put("ConnRetryBackoffMultiplier", NUMBER_IMAGE_NAME);
		imageMappings.put("ConnRetryInitialDelay",      NUMBER_IMAGE_NAME);
		imageMappings.put("DatalinkControlChunks",      NUMBER_IMAGE_NAME);
		imageMappings.put("DatalinkReleaseDelay",       NUMBER_IMAGE_NAME);
		imageMappings.put("DefaultToIPv6",              BOOLEAN_IMAGE_NAME);
		imageMappings.put("EnableNagleAlgorithm",       BOOLEAN_IMAGE_NAME);
		imageMappings.put("genspec",                    NODE_IMAGE_NAME);
		imageMappings.put("GroupAddress",               STRING_IMAGE_NAME);
		imageMappings.put("Instance",                   INSTANCE_IMAGE_NAME);
		imageMappings.put("Instances",                  NODE_IMAGE_NAME);
		imageMappings.put("LocalAddressString",         STRING_IMAGE_NAME);
		imageMappings.put("MaxOutputPausePeriod",       NUMBER_IMAGE_NAME);
		imageMappings.put("MaxPacketSize",              NUMBER_IMAGE_NAME);
		imageMappings.put("MaxSamplesPerPacket",        NUMBER_IMAGE_NAME);
		imageMappings.put("ModelFile",                  PACKAGE_IMAGE_NAME);
		imageMappings.put("MulticastConfig",            PACKAGE_IMAGE_NAME);
		imageMappings.put("NackDepth",                  NUMBER_IMAGE_NAME);
		imageMappings.put("NakInterval",                NUMBER_IMAGE_NAME);
		imageMappings.put("NakMax",                     NUMBER_IMAGE_NAME);
		imageMappings.put("NakTimeout",                 NUMBER_IMAGE_NAME);
		imageMappings.put("OptimumPacketSize",          NUMBER_IMAGE_NAME);
		imageMappings.put("PassiveConnectDuration",     NUMBER_IMAGE_NAME);
		imageMappings.put("PassiveReconnectDuration",   NUMBER_IMAGE_NAME);
		imageMappings.put("PortOffset",                 NUMBER_IMAGE_NAME);
		imageMappings.put("QueueInitialPool",           NUMBER_IMAGE_NAME);
		imageMappings.put("QueueMessagesPerPool",       NUMBER_IMAGE_NAME);
		imageMappings.put("RcvBufferSize",              NUMBER_IMAGE_NAME);
		imageMappings.put("Reliable",                   BOOLEAN_IMAGE_NAME);
		imageMappings.put("SwapBytes",                  BOOLEAN_IMAGE_NAME);
		imageMappings.put("SynBackoff",                 NUMBER_IMAGE_NAME);
		imageMappings.put("SynInterval",                NUMBER_IMAGE_NAME);
		imageMappings.put("SynTimeout",                 NUMBER_IMAGE_NAME);
		imageMappings.put("TargetDir",                  PACKAGE_IMAGE_NAME);
		imageMappings.put("TcpConfig",                  PACKAGE_IMAGE_NAME);
		imageMappings.put("ThreadPerConnection",        BOOLEAN_IMAGE_NAME);
		imageMappings.put("Transport",                  PACKAGE_IMAGE_NAME);
		imageMappings.put("TransportOffset",            NUMBER_IMAGE_NAME);
		imageMappings.put("TTL",                        NUMBER_IMAGE_NAME);
		imageMappings.put("UdpConfig",                  PACKAGE_IMAGE_NAME);
		
		// org.opendds.modeling.sdk.model.editor		
		imageMappings.put("GeneratorModelFile",         GENERATOR_IMAGE_NAME);
		imageMappings.put("NewGenerator",               GENERATOR_LARGE_IMAGE_NAME);
	}

	/**
	 * The activator singleton.
	 */
	private static Implementation plugin;
	
	/**
	 * The constructor
	 */
	public Plugin() {
		super
		  (new ResourceLocator [] {
		   });
	}

	/**
	 * Returns the singleton instance of the Eclipse plug-in.
	 */
	@Override
	public ResourceLocator getPluginResourceLocator() {
		return plugin;
	}
	
	/**
	 * Maps items to images.
	 */
	public String imageMapping( String item) {
		if( imageMappings.containsKey(item)) {
			return imageMappings.get(item);
			
		} else {
			return item;
		}
	}

	/**
	 * Returns the singleton instance of the Eclipse plug-in.
	 */
	public static Implementation getPlugin() {
		return plugin;
	}

	/**
	 * The actual implementation of the Eclipse Plugin.
	 */
	public static class Implementation extends EclipsePlugin {
		/**
		 * Creates an instance.
		 */
		public Implementation() {
			super();

			// Remember the static instance.
			//
			plugin = this;
		}
	}
	
}
