package org.opendds.modeling.diagram.dcpslib.edit.commands;


import java.util.HashMap;
import java.util.Map;

import org.eclipse.gmf.runtime.emf.type.core.requests.CreateElementRequest;
import com.ociweb.gmf.edit.commands.AddReferenceCommand;

/**
 * @generated NOT
 */
public class QosPolicyReferCommand extends AddReferenceCommand {

	private static final Map<String, String> classNameToRefName =
		new HashMap<String, String>();

	static {
		classNameToRefName.put("deadlineQosPolicy", "deadline");
		classNameToRefName.put("destinationOrderQosPolicy", "destination_order");
		classNameToRefName.put("durabilityQosPolicy", "durability");
		classNameToRefName.put("dsQosPolicy", "durability_service");
		classNameToRefName.put("efQosPolicy", "entity_factory");
		classNameToRefName.put("gdQosPolicy", "group_data");
		classNameToRefName.put("historyQosPolicy", "history");
		classNameToRefName.put("lbQosPolicy", "latency_budget");
		classNameToRefName.put("livelinessQosPolicy", "liveliness");
		classNameToRefName.put("ownershipQosPolicy", "ownership");
		classNameToRefName.put("osQosPolicy", "ownership_strength");
		classNameToRefName.put("partitionQosPolicy", "partition");
		classNameToRefName.put("presentationQosPolicy", "presentation");
		classNameToRefName.put("rdlQosPolicy", "reader_data_lifecycle");
		classNameToRefName.put("reliabilityQosPolicy", "reliability");
		classNameToRefName.put("rlQosPolicy", "resource_limits");
		classNameToRefName.put("tbfQosPolicy", "time_based_filter");
		classNameToRefName.put("tpQosPolicy", "transport_priority");
		classNameToRefName.put("udQosPolicy", "user_data");
		classNameToRefName.put("wdlQosPolicy", "writer_data_lifecycle");
		classNameToRefName.put("lifespanQosPolicy", "lifespan");
	}


	public QosPolicyReferCommand(CreateElementRequest req) {
		super(req, classNameToRefName);
	}

}
