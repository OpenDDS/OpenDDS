package org.opendds.modeling.diagram.dcpslib.edit.parts;

import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EPackage;
import org.opendds.modeling.model.opendds.OpenDDSPackage;

/**
 * Helper class for filter policy compartments based on whether domain entity
 * "owns" or does not own the policy.
 */
public class CompartmentChildrenFinder {

	/**
	 * Filter compartment's policies that are (ultimately) owned by a DcpsLib.
	 */
	static java.util.List filterPoliciesOwnedByDcpsLib(java.util.List modelChildren) {

		return com.ociweb.gmf.edit.parts.ModelChildrenFilter
		.filterChildrenByOwner(getDcpsLibClass(), modelChildren);

	}
	
	/**
	 * Filter compartment's policies that are not owned by the same DcpsLib that (ultimately) owns domainEntity.
	 */
	static java.util.List filterPoliciesNotOwnedByDcpsLib(EObject domainEntity, java.util.List modelChildren) {

		EObject dcpsLib = com.ociweb.emf.util.ObjectsFinder.findObjectInContainmentTree(domainEntity, getDcpsLibClass());
		
		java.util.List filteredChildren = null;
		
		if (dcpsLib != null) {
			filteredChildren = com.ociweb.gmf.edit.parts.ModelChildrenFilter
			.filterChildrenWithObjectNotInContainmentTree(dcpsLib, modelChildren);
		}
		
		return filteredChildren;
	}
	
	private static EClass getDcpsLibClass() {
		EPackage dcpsLibsPackage = EPackage.Registry.INSTANCE
		.getEPackage(OpenDDSPackage.eNS_URI);
		return (EClass) dcpsLibsPackage
		.getEClassifier("DcpsLib");
	}
}
