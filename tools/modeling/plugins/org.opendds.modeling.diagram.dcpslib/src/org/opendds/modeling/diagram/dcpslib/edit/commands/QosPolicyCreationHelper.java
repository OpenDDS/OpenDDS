package org.opendds.modeling.diagram.dcpslib.edit.commands;

import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EPackage;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.opendds.modeling.model.opendds.DcpsLib;
import org.opendds.modeling.model.opendds.OpenDDSPackage;
import org.opendds.modeling.model.qos.QosPolicy;

/**
 * generated NOT
 */
class QosPolicyCreationHelper {

	static <PolicyType extends QosPolicy> void addPolicy(PolicyType newElement, String policyAssociationName, EObject referrer) {

		// Add to owner, which we know must be a DcpsLib
		DcpsLib owner = (DcpsLib) com.ociweb.emf.util.ObjectsFinder.findObjectInContainmentTree(referrer, getDcpsLibClass());
		owner.getPolicies().add(newElement);

		// Add to referrer
		EStructuralFeature structuralFeature = referrer.eClass().getEStructuralFeature(policyAssociationName);
		referrer.eSet(structuralFeature, newElement);

	}

	private static EClass getDcpsLibClass() {
		EPackage dcpsLibsPackage = EPackage.Registry.INSTANCE.getEPackage(OpenDDSPackage.eNS_URI);
		return (EClass) dcpsLibsPackage.getEClassifier("DcpsLib");
	}

}
