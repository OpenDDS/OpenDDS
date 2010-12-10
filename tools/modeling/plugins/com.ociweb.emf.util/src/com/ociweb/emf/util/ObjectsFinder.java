package com.ociweb.emf.util;

import org.eclipse.emf.ecore.EObject;

public class ObjectsFinder {

	/**
	 * Find the object that is the root container for an object.
	 * @return Root container or null if object does not have
	 * a container.
	 */
	public static EObject findRootContainerObject(EObject obj) {

		EObject rootContainer = obj.eContainer();
		while (rootContainer != null) {
			EObject container = rootContainer.eContainer();
			if (container == null) {
				break;
			}
			rootContainer = container;
		}
		return rootContainer;
	}

	/**
	 * Answer if one object is in the container tree for another object.
	 * @param obj The object whose containers in the containment tree should be checked. 
	 * @param containerCandidate The object to test if it is in the containment tree.
	 * @return true if containerCandidate is in the containment tree for obj.
	 */
	public static boolean hasObjectInContainmentTree(EObject obj, EObject containerCandidate) {
		boolean candidateMatches = false;
		
		EObject container = obj.eContainer();
		while (container != null) {
			if (container == containerCandidate) {
				return true;
			}
			container = container.eContainer();
		}
		return candidateMatches;
	}
}
