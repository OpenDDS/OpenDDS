package org.opendds.modeling.diagram.dcpslib.edit.commands;

import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.opendds.modeling.model.opendds.DcpsLib;
import org.opendds.modeling.model.opendds.qosPolicy;

/**
 * generated NOT
 */
class QosPolicyCreationHelper {

	static <PolicyType extends qosPolicy> void addPolicy(PolicyType newElement, String policyAssociationName, EObject referrer) {

		// Add to owner, which we know must be a DcpsLib
		DcpsLib owner = null;

		EObject container = referrer.eContainer();
		if (container instanceof DcpsLib) {
			owner = (DcpsLib) container;
		}
		else {
			container = container.eContainer();
			if (container instanceof DcpsLib) {
				owner = (DcpsLib) container;
			}
			else {
				container = container.eContainer();
				if (container instanceof DcpsLib) {
					owner = (DcpsLib) container;
				}
			}
		}
		owner.getPolicies().add(newElement);

		// Add to referrer
		EStructuralFeature structuralFeature = referrer.eClass().getEStructuralFeature(policyAssociationName);
		referrer.eSet(structuralFeature, newElement);
		
	}
}
