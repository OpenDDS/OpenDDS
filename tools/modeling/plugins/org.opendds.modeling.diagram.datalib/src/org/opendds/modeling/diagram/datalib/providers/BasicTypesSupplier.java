package org.opendds.modeling.diagram.datalib.providers;

import java.util.Iterator;

import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EClassifier;
import org.eclipse.emf.ecore.EFactory;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EPackage;
import org.opendds.modeling.common.gmf.BasicTypeIdentifier;
import org.opendds.modeling.model.opendds.BasicTypes;
import org.opendds.modeling.model.opendds.OpenDDSFactory;
import org.opendds.modeling.model.opendds.OpenDDSModel;
import org.opendds.modeling.model.opendds.OpenDDSPackage;
import org.opendds.modeling.model.types.Simple;
import org.opendds.modeling.model.types.Type;
import org.opendds.modeling.model.types.TypesFactory;
import org.opendds.modeling.model.types.TypesPackage;



/**
 * For Field types used in Structs and Unions, supply basic type instances that
 * are independent of basic types defined elsewhere in a DataLib. This removes
 * the dependency between a basic Field type and basic types defined elsewhere
 * that may be later removed.
 * Basic types used here are Type-derived types that do not require any attributes.
 * This includes Simple types except Enum, and String and WString with length of 0.
 */
class BasicTypesSupplier {

	static final EPackage typesPackage = EPackage.Registry.INSTANCE.getEPackage(TypesPackage.eNS_URI);
	static final EPackage openDDSPackage = EPackage.Registry.INSTANCE.getEPackage(OpenDDSPackage.eNS_URI);

	/**
	 * Get the instance of a basic type whose class is named typeName.
	 * @param openDDSModel The container to hold the BasicTypes
	 * @param typeName The name of the basic to return
	 * @return The instance whose class has name typeName, or null if non-existent.
	 */
	static Type create(OpenDDSModel openDDSModel, String typeName) {

		if (getTypeClass(typeName) == null) {
			return null;
		}

		Type basicType = null;
		BasicTypes basicTypes = null;

		if (openDDSModel.getBasicTypes() == null) {
			EFactory openDDSFactory = OpenDDSFactory.eINSTANCE;

			// Create the BasicTypes class
			EClass basicTypesClass = (EClass) openDDSPackage.getEClassifier("BasicTypes");
			basicTypes = (BasicTypes) openDDSFactory.create(basicTypesClass);

			// Add the BasicTypes to the package
			openDDSModel.setBasicTypes(basicTypes);

			// Create the allowed basic types
			EFactory typesFactory = TypesFactory.eINSTANCE;
			for (Iterator iter = typesPackage.getEClassifiers().iterator(); iter.hasNext(); ) {
				EClassifier classifier = (EClassifier) iter.next();
				if (classifier instanceof EClass) {
					EClass typeClass = (EClass) classifier;
					if (BasicTypeIdentifier.isBasic(typeClass)) {
						Type basicTypeObject = (Type) typesFactory.create(typeClass);
						basicTypes.getTypes().add(basicTypeObject);
						if (typeClass.getName().equals(typeName)) {
							basicType = basicTypeObject;
						}
					}
				}
			}
		}
		return basicType;
	}

	/**
	 * If OpenDDSModel.basicTypes exists and it contains an object whose type
	 * is named typeCandidateName, return that object. Otherwise return null.
	 */
	static Type getType(OpenDDSModel openDDSModel, String typeCandidateName) {
		BasicTypes basicTypes = openDDSModel.getBasicTypes();
		if (basicTypes != null) {
			for (Type basicType: basicTypes.getTypes()) {
				if (basicType.eClass().getName().equals(typeCandidateName)) {
					return basicType;
				}
			}
		}
		return null;
	}

	/**
	 * Given the name of a type, return the EClass of the type is it exists.
	 * Otherwise return null.
	 */
	static EClass getTypeClass(String typeCandidateName) {
		EClass typeClass = null;
		EClass typeCandidateClass = (EClass) typesPackage.getEClassifier(typeCandidateName);
		if (typeCandidateClass != null && BasicTypeIdentifier.isBasic(typeCandidateClass)) {
			typeClass = typeCandidateClass;
		}
		return typeClass;
	}
}
