package org.opendds.modeling.diagram.dcpslib.part;

import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

/**
 * Given the name of a class derived from QosPolicy, return the
 * name of the reference to the policy on a domain entity.
 */
public class QosPolicyClassNameToRefName {

	private static final Map<String, String> classNameToRefName;

	static {
		Map<String, String> strTostr =  new HashMap<String, String>();
		strTostr.put("deadlineQosPolicy", "deadline");
		strTostr.put("destinationOrderQosPolicy", "destination_order");
		strTostr.put("durabilityQosPolicy", "durability");
		strTostr.put("dsQosPolicy", "durability_service");
		strTostr.put("efQosPolicy", "entity_factory");
		strTostr.put("gdQosPolicy", "group_data");
		strTostr.put("historyQosPolicy", "history");
		strTostr.put("lbQosPolicy", "latency_budget");
		strTostr.put("livelinessQosPolicy", "liveliness");
		strTostr.put("ownershipQosPolicy", "ownership");
		strTostr.put("osQosPolicy", "ownership_strength");
		strTostr.put("partitionQosPolicy", "partition");
		strTostr.put("presentationQosPolicy", "presentation");
		strTostr.put("rdlQosPolicy", "reader_data_lifecycle");
		strTostr.put("reliabilityQosPolicy", "reliability");
		strTostr.put("rlQosPolicy", "resource_limits");
		strTostr.put("tbfQosPolicy", "time_based_filter");
		strTostr.put("tpQosPolicy", "transport_priority");
		strTostr.put("udQosPolicy", "user_data");
		strTostr.put("wdlQosPolicy", "writer_data_lifecycle");
		strTostr.put("lifespanQosPolicy", "lifespan");
		classNameToRefName = Collections.unmodifiableMap(strTostr);
	}

	public static String getRefName(String className) {
		return classNameToRefName.get(className);
	}
}
