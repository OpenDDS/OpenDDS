package org.opendds.modeling.model.opendds.diagram.middlewarelibrary.part;

import org.opendds.modeling.model.opendds.diagram.middlewarelibrary.edit.parts.PublisherEditPart;
import org.opendds.modeling.model.opendds.diagram.middlewarelibrary.edit.parts.PublisherQoSPoliciesCustomEditPart;

/**
 * generate NOT
 */
public class OpenDDSMiddlewareLibraryCreateQosPolicyActionPublisher extends
OpenDDSMiddlewareLibraryCreateQosPolicyAction<PublisherQoSPoliciesCustomEditPart>
{

	public OpenDDSMiddlewareLibraryCreateQosPolicyActionPublisher() {
		super(PublisherEditPart.class);
	}

}
