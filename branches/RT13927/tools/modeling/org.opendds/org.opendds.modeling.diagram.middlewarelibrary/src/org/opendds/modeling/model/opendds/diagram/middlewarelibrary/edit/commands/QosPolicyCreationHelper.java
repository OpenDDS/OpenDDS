package org.opendds.modeling.model.opendds.diagram.middlewarelibrary.edit.commands;

import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.opendds.modeling.model.opendds.MiddlewareLibrary;
import org.opendds.modeling.model.opendds.qosPolicy;

/**
 * generated NOT
 */
class QosPolicyCreationHelper {

	static <PolicyType extends qosPolicy> void addPolicy(PolicyType newElement, String policyAssociationName, EObject referrer) {

		// Add to owner, which we know must be a MiddlewareLibrary
		MiddlewareLibrary owner = null;

		EObject container = referrer.eContainer();
		if (container instanceof MiddlewareLibrary) {
			owner = (MiddlewareLibrary) container;
		}
		else {
			container = container.eContainer();
			if (container instanceof MiddlewareLibrary) {
				owner = (MiddlewareLibrary) container;
			}
			else {
				container = container.eContainer();
				if (container instanceof MiddlewareLibrary) {
					owner = (MiddlewareLibrary) container;
				}
			}
		}
		owner.getPolicies().add(newElement);

		// Add to referrer
		EStructuralFeature structuralFeature = referrer.eClass().getEStructuralFeature(policyAssociationName);
		referrer.eSet(structuralFeature, newElement);
		
	}
}
