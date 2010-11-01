package org.opendds.modeling.model.opendds.diagram.middlewarelib.part;

import org.opendds.modeling.model.opendds.diagram.middlewarelib.edit.parts.SubscriberEditPart;
import org.opendds.modeling.model.opendds.diagram.middlewarelib.edit.parts.SubscriberQoSPoliciesCustomEditPart;

/**
 * generate NOT
 */
public class OpenDDSMiddlewareLibCreateQosPolicyActionSubscriber extends
OpenDDSMiddlewareLibCreateQosPolicyAction<SubscriberQoSPoliciesCustomEditPart>
{

	public OpenDDSMiddlewareLibCreateQosPolicyActionSubscriber() {
		super(SubscriberEditPart.class);
	}

}
