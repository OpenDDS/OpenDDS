package org.opendds.modeling.diagram.dcpslib.part;

import org.eclipse.emf.ecore.EObject;
import org.opendds.modeling.model.opendds.PolicyLib;
import org.opendds.modeling.model.qos.QosPolicy;


public class QosPolicyLabeler extends com.ociweb.gmf.dialogs.ObjectLabeler<QosPolicy> {

	@Override
	public String getLabel(QosPolicy policy) {
		String policyName = "";
		
		// Qualify by policy owner since a policy library may be in the same resource
		// as the element using the policy.
		EObject policyOwner = policy.eContainer();
		if (policyOwner instanceof PolicyLib ) {
			policyName = ((PolicyLib) policyOwner).getName() + ".";
		}
		policyName += policy.getName();
		return "\u00ab" + policy.eClass().getName() + "\u00bb " + policyName;
	}

}
