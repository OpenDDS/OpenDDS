package org.opendds.modeling.model.opendds.diagram.middlewarelibrary.part;

import org.opendds.modeling.model.opendds.diagram.middlewarelibrary.edit.parts.DomainParticipantEditPart;
import org.opendds.modeling.model.opendds.diagram.middlewarelibrary.edit.parts.DomainParticipantQoSPoliciesCustomEditPart;

/**
 * generate NOT
 */
public class OpenDDSMiddlewareLibraryCreateQosPolicyActionDomainParticipant extends
OpenDDSMiddlewareLibraryCreateQosPolicyAction<DomainParticipantQoSPoliciesCustomEditPart>
{

	public OpenDDSMiddlewareLibraryCreateQosPolicyActionDomainParticipant() {
		super(DomainParticipantEditPart.class);
	}

}
