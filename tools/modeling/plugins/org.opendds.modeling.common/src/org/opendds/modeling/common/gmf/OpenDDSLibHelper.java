package org.opendds.modeling.common.gmf;

import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EPackage;
import org.opendds.modeling.model.opendds.OpenDDSLib;
import org.opendds.modeling.model.opendds.OpenDDSPackage;

public class OpenDDSLibHelper {

	static EClass OPENDDSLIBCLASS = getOpenDDSLibClass();

	static public OpenDDSLib findLib(EObject element) {
		EObject lib = com.ociweb.emf.util.ObjectsFinder.findObjectInContainmentTree(element, OPENDDSLIBCLASS);
		if (lib == null) {
			return null;
		}
		assert lib instanceof OpenDDSLib;
		return (OpenDDSLib) lib;
	}

	/**
	 * Answers if two elements are contained in the same library.
	 */
	static public boolean areElementsInSameLib(EObject element1, EObject element2) {
		OpenDDSLib lib1 = findLib(element1);
		if (lib1 == null) {
			return false;
		}
		OpenDDSLib lib2 = findLib(element2);
		return lib1 == lib2;
	}

	private static EClass getOpenDDSLibClass() {
		EPackage dcpsLibsPackage = EPackage.Registry.INSTANCE
		.getEPackage(OpenDDSPackage.eNS_URI);
		return (EClass) dcpsLibsPackage
		.getEClassifier("OpenDDSLib");
	}
}
