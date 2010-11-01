package org.opendds.modeling.model.opendds.diagram.middlewarelib.part;

import org.opendds.modeling.model.opendds.diagram.middlewarelib.edit.parts.DomainParticipantEditPart;
import org.opendds.modeling.model.opendds.diagram.middlewarelib.edit.parts.DomainParticipantQoSPoliciesCustomEditPart;

/**
 * generate NOT
 */
public class OpenDDSMiddlewareLibCreateQosPolicyActionDomainParticipant extends
OpenDDSMiddlewareLibCreateQosPolicyAction<DomainParticipantQoSPoliciesCustomEditPart>
{

	public OpenDDSMiddlewareLibCreateQosPolicyActionDomainParticipant() {
		super(DomainParticipantEditPart.class);
	}

}
