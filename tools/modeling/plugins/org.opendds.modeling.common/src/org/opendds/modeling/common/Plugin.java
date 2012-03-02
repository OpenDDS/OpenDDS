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
	
	// EMF model and GMF diagrams
    protected static final String DATATYPE_IMAGE_NAME        = "DataType";
    protected static final String DCPS_IMAGE_NAME            = "Dcps";
    protected static final String INPUTPIN_IMAGE_NAME        = "InputPin";
    protected static final String OPENDDS_DIAGRAM_FILE_IMAGE_NAME = "OpenDDSDiagramFile";
    protected static final String OPENDDS_MODEL_FILE_IMAGE_NAME = "OpenDDSModelFile";
    protected static final String OPENDDS_SDK_LIB_IMAGE_NAME = "OpenDDSSdkLib";
    protected static final String OUTPUTPIN_IMAGE_NAME       = "OutputPin";
    protected static final String QOSPOLICY_IMAGE_NAME       = "QosPolicy";
    protected static final String CLOCK_IMAGE_NAME           = "clock";
    protected static final String UMLPACKAGE_IMAGE_NAME      = "UmlPackage";

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
		imageMappings.put("TransportConfig", NODE_IMAGE_NAME);
		imageMappings.put("TransportImpl", NODE_IMAGE_NAME);
		imageMappings.put("TcpTransport", NODE_IMAGE_NAME);
		imageMappings.put("UdpTransport", NODE_IMAGE_NAME);
		imageMappings.put("MulticastTransport", NODE_IMAGE_NAME);
		
		// org.opendds.modeling.sdk.model.editor		
		imageMappings.put("GeneratorModelFile",         GENERATOR_IMAGE_NAME);
		imageMappings.put("NewGenerator",               GENERATOR_LARGE_IMAGE_NAME);
		
		// OpenDDSModel
		imageMappings.put("DataLib",                    OPENDDS_SDK_LIB_IMAGE_NAME);
		imageMappings.put("DcpsLib",                    OPENDDS_SDK_LIB_IMAGE_NAME);
		imageMappings.put("PolicyLib",                  OPENDDS_SDK_LIB_IMAGE_NAME);
		imageMappings.put("OpenDDSModel",               OPENDDS_MODEL_FILE_IMAGE_NAME);
		imageMappings.put("LibPackage",                 UMLPACKAGE_IMAGE_NAME);

		// DataLib
		imageMappings.put("Array",                      DATATYPE_IMAGE_NAME);
		imageMappings.put("BasicTypes",                 DATATYPE_IMAGE_NAME);
		imageMappings.put("Boolean",                    DATATYPE_IMAGE_NAME);
		imageMappings.put("Branch",                     DATATYPE_IMAGE_NAME);
		imageMappings.put("Case",                       DATATYPE_IMAGE_NAME);
		imageMappings.put("Char",                       DATATYPE_IMAGE_NAME);
		imageMappings.put("Double",                     DATATYPE_IMAGE_NAME);
		imageMappings.put("Enum",                       DATATYPE_IMAGE_NAME);
		imageMappings.put("Field",                      DATATYPE_IMAGE_NAME);
		imageMappings.put("Float",                      DATATYPE_IMAGE_NAME);
		imageMappings.put("Key",                        DATATYPE_IMAGE_NAME);
		imageMappings.put("Long",                       DATATYPE_IMAGE_NAME);
		imageMappings.put("LongDouble",                 DATATYPE_IMAGE_NAME);
		imageMappings.put("LongLong",                   DATATYPE_IMAGE_NAME);
		imageMappings.put("Octet",                      DATATYPE_IMAGE_NAME);
		imageMappings.put("Sequence",                   DATATYPE_IMAGE_NAME);
		imageMappings.put("Short",                      DATATYPE_IMAGE_NAME);
		imageMappings.put("String",                     DATATYPE_IMAGE_NAME);
		imageMappings.put("Struct",                     DATATYPE_IMAGE_NAME);
		imageMappings.put("Typedef",                    DATATYPE_IMAGE_NAME);
		imageMappings.put("ULong",                      DATATYPE_IMAGE_NAME);
		imageMappings.put("ULongLong",                  DATATYPE_IMAGE_NAME);
		imageMappings.put("UShort",                     DATATYPE_IMAGE_NAME);
		imageMappings.put("Union",                      DATATYPE_IMAGE_NAME);
		imageMappings.put("WChar",                      DATATYPE_IMAGE_NAME);
		imageMappings.put("WString",                    DATATYPE_IMAGE_NAME);

		// DcpsLib
		imageMappings.put("ContentFilteredTopic",       DCPS_IMAGE_NAME);
		imageMappings.put("domain",                     DCPS_IMAGE_NAME);
		imageMappings.put("Domain",                     DCPS_IMAGE_NAME);
		imageMappings.put("domainParticipant",          DCPS_IMAGE_NAME);
		imageMappings.put("DomainParticipant",          DCPS_IMAGE_NAME);
		imageMappings.put("MultiTopic",                 DCPS_IMAGE_NAME);
		imageMappings.put("topic",                      DCPS_IMAGE_NAME);
		imageMappings.put("Topic",                      DCPS_IMAGE_NAME);
		imageMappings.put("dataReader",                 INPUTPIN_IMAGE_NAME);
		imageMappings.put("DataReader",                 INPUTPIN_IMAGE_NAME);
		imageMappings.put("subscriber",                 INPUTPIN_IMAGE_NAME);
		imageMappings.put("Subscriber",                 INPUTPIN_IMAGE_NAME);
		imageMappings.put("dataWriter",                 OUTPUTPIN_IMAGE_NAME);
		imageMappings.put("DataWriter",                 OUTPUTPIN_IMAGE_NAME);
		imageMappings.put("publisher",                  OUTPUTPIN_IMAGE_NAME);
		imageMappings.put("Publisher",                  OUTPUTPIN_IMAGE_NAME);

		// PolicyLib
		imageMappings.put("deadlineQosPolicy",          QOSPOLICY_IMAGE_NAME);
		imageMappings.put("DeadlineQosPolicy",          QOSPOLICY_IMAGE_NAME);
		imageMappings.put("destinationOrderQosPolicy",  QOSPOLICY_IMAGE_NAME);
		imageMappings.put("DestinationOrderQosPolicy",  QOSPOLICY_IMAGE_NAME);
		imageMappings.put("dsQosPolicy",                QOSPOLICY_IMAGE_NAME);
		imageMappings.put("durabilityQosPolicy",        QOSPOLICY_IMAGE_NAME);
		imageMappings.put("DurabilityQosPolicy",        QOSPOLICY_IMAGE_NAME);
		imageMappings.put("DurabilityServiceQosPolicy", QOSPOLICY_IMAGE_NAME);
		imageMappings.put("efQosPolicy",                QOSPOLICY_IMAGE_NAME);
		imageMappings.put("EntityFactoryQosPolicy",     QOSPOLICY_IMAGE_NAME);
		imageMappings.put("gdQosPolicy",                QOSPOLICY_IMAGE_NAME);
		imageMappings.put("GroupDataQosPolicy",         QOSPOLICY_IMAGE_NAME);
		imageMappings.put("historyQosPolicy",           QOSPOLICY_IMAGE_NAME);
		imageMappings.put("HistoryQosPolicy",           QOSPOLICY_IMAGE_NAME);
		imageMappings.put("LatencyBudgetQosPolicy",     QOSPOLICY_IMAGE_NAME);
		imageMappings.put("lbQosPolicy",                QOSPOLICY_IMAGE_NAME);
		imageMappings.put("lifespanQosPolicy",          QOSPOLICY_IMAGE_NAME);
		imageMappings.put("LifespanQosPolicy",          QOSPOLICY_IMAGE_NAME);
		imageMappings.put("livelinessQosPolicy",        QOSPOLICY_IMAGE_NAME);
		imageMappings.put("LivelinessQosPolicy",        QOSPOLICY_IMAGE_NAME);
		imageMappings.put("osQosPolicy",                QOSPOLICY_IMAGE_NAME);
		imageMappings.put("ownershipQosPolicy",         QOSPOLICY_IMAGE_NAME);
		imageMappings.put("OwnershipQosPolicy",         QOSPOLICY_IMAGE_NAME);
		imageMappings.put("OwnershipStrengthQosPolicy", QOSPOLICY_IMAGE_NAME);
		imageMappings.put("partitionQosPolicy",         QOSPOLICY_IMAGE_NAME);
		imageMappings.put("PartitionQosPolicy",         QOSPOLICY_IMAGE_NAME);
		imageMappings.put("presentationQosPolicy",      QOSPOLICY_IMAGE_NAME);
		imageMappings.put("rdlQosPolicy",               QOSPOLICY_IMAGE_NAME);
		imageMappings.put("ReaderDataLifecycleQosPolicy", QOSPOLICY_IMAGE_NAME);
		imageMappings.put("reliabilityQosPolicy",       QOSPOLICY_IMAGE_NAME);
		imageMappings.put("ReliabilityQosPolicy",       QOSPOLICY_IMAGE_NAME);
		imageMappings.put("ResourceLimitsQosPolicy",    QOSPOLICY_IMAGE_NAME);
		imageMappings.put("rlQosPolicy",                QOSPOLICY_IMAGE_NAME);
		imageMappings.put("tbfQosPolicy",               QOSPOLICY_IMAGE_NAME);
		imageMappings.put("tdQosPolicy",                QOSPOLICY_IMAGE_NAME);
		imageMappings.put("TimeBasedFilterQosPolicy",   QOSPOLICY_IMAGE_NAME);
		imageMappings.put("TopicDataQosPolicy",         QOSPOLICY_IMAGE_NAME);
		imageMappings.put("tpQosPolicy",                QOSPOLICY_IMAGE_NAME);
		imageMappings.put("TransportPriorityQosPolicy", QOSPOLICY_IMAGE_NAME);
		imageMappings.put("udQosPolicy",                QOSPOLICY_IMAGE_NAME);
		imageMappings.put("UserDataQosPolicy",          QOSPOLICY_IMAGE_NAME);
		imageMappings.put("wdlQosPolicy",               QOSPOLICY_IMAGE_NAME);
		imageMappings.put("WriterDataLifecycleQosPolicy", QOSPOLICY_IMAGE_NAME);
		imageMappings.put("Period",                     CLOCK_IMAGE_NAME);
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
