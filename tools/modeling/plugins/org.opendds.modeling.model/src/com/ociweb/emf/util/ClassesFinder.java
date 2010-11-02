package com.ociweb.emf.util;

import org.eclipse.emf.common.util.BasicEList;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EClassifier;
import org.eclipse.emf.ecore.EPackage;

public class ClassesFinder {

	/**
	 * Find all the classes in a package that inherit from a super type.
	 */
	public static EList<EClass> findDerived(EClass superType, EPackage ePackage) {
		EList<EClass> derived = new BasicEList<EClass>();
		for (EClassifier classifier : ePackage.getEClassifiers()) {
			if (classifier instanceof EClass) {
				EClass clazz = (EClass) classifier;
				if (superType.isSuperTypeOf(clazz)) {
					derived.add(clazz);
				}
			}
		}
		return derived;
	}
}
