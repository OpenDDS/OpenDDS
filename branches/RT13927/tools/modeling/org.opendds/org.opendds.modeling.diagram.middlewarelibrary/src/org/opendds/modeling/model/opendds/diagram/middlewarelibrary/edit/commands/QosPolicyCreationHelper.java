package org.opendds.modeling.model.opendds.diagram.middlewarelib.edit.commands;

import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.opendds.modeling.model.opendds.MiddlewareLib;
import org.opendds.modeling.model.opendds.qosPolicy;

/**
 * generated NOT
 */
class QosPolicyCreationHelper {

	static <PolicyType extends qosPolicy> void addPolicy(PolicyType newElement, String policyAssociationName, EObject referrer) {

		// Add to owner, which we know must be a MiddlewareLib
		MiddlewareLib owner = null;

		EObject container = referrer.eContainer();
		if (container instanceof MiddlewareLib) {
			owner = (MiddlewareLib) container;
		}
		else {
			container = container.eContainer();
			if (container instanceof MiddlewareLib) {
				owner = (MiddlewareLib) container;
			}
			else {
				container = container.eContainer();
				if (container instanceof MiddlewareLib) {
					owner = (MiddlewareLib) container;
				}
			}
		}
		owner.getPolicies().add(newElement);

		// Add to referrer
		EStructuralFeature structuralFeature = referrer.eClass().getEStructuralFeature(policyAssociationName);
		referrer.eSet(structuralFeature, newElement);
		
	}
}
