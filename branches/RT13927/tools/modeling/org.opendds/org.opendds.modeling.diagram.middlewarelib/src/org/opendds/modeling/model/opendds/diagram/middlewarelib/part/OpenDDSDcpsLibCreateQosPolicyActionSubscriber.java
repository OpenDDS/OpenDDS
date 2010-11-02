package org.opendds.modeling.model.opendds.diagram.dcpslib.part;

import org.opendds.modeling.model.opendds.diagram.dcpslib.edit.parts.SubscriberEditPart;
import org.opendds.modeling.model.opendds.diagram.dcpslib.edit.parts.SubscriberQoSPoliciesCustomEditPart;

/**
 * generate NOT
 */
public class OpenDDSDcpsLibCreateQosPolicyActionSubscriber extends
OpenDDSDcpsLibCreateQosPolicyAction<SubscriberQoSPoliciesCustomEditPart>
{

	public OpenDDSDcpsLibCreateQosPolicyActionSubscriber() {
		super(SubscriberEditPart.class);
	}

}
