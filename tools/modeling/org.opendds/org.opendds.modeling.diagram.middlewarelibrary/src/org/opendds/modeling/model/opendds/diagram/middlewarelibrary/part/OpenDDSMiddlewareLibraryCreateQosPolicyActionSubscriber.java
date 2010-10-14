package org.opendds.modeling.model.opendds.diagram.middlewarelibrary.part;

import org.opendds.modeling.model.opendds.diagram.middlewarelibrary.edit.parts.SubscriberEditPart;
import org.opendds.modeling.model.opendds.diagram.middlewarelibrary.edit.parts.SubscriberQoSPoliciesCustomEditPart;

/**
 * generate NOT
 */
public class OpenDDSMiddlewareLibraryCreateQosPolicyActionSubscriber extends
OpenDDSMiddlewareLibraryCreateQosPolicyAction<SubscriberQoSPoliciesCustomEditPart>
{

	public OpenDDSMiddlewareLibraryCreateQosPolicyActionSubscriber() {
		super(SubscriberEditPart.class);
	}

}
