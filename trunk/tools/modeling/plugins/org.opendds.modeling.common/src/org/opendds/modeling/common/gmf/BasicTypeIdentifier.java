package org.opendds.modeling.common.gmf;

import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EPackage;
import org.opendds.modeling.model.types.TypesPackage;

public class BasicTypeIdentifier {

	static final EPackage typesPackage = EPackage.Registry.INSTANCE.getEPackage(TypesPackage.eNS_URI);

	public static boolean isBasic(EClass type) {
		EClass simpleClass = (EClass) typesPackage.getEClassifier("Simple");
		if (simpleClass.isSuperTypeOf(type) && !type.isAbstract()) {
			if (type.getName() == "Enum") {
				return false;
			}
			return true;
		} else if (type.getName().equals("String") ||
				type.getName().equals("WString")) {
			return true;
		}
		return false;
	}

}
