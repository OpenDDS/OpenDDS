package org.opendds.modeling.model.opendds.diagram.middlewarelib.part;

import org.opendds.modeling.model.opendds.diagram.middlewarelib.edit.parts.PublisherEditPart;
import org.opendds.modeling.model.opendds.diagram.middlewarelib.edit.parts.PublisherQoSPoliciesCustomEditPart;

/**
 * generate NOT
 */
public class OpenDDSMiddlewareLibCreateQosPolicyActionPublisher extends
OpenDDSMiddlewareLibCreateQosPolicyAction<PublisherQoSPoliciesCustomEditPart>
{

	public OpenDDSMiddlewareLibCreateQosPolicyActionPublisher() {
		super(PublisherEditPart.class);
	}

}
