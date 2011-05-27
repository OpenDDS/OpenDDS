package org.opendds.modeling.diagram.dcpslib.part;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;

import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EPackage;
import org.eclipse.gmf.runtime.diagram.ui.editparts.ListCompartmentEditPart;
import org.eclipse.gmf.runtime.emf.type.core.IElementType;
import org.eclipse.gmf.runtime.notation.View;
import org.eclipse.jface.action.IAction;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.MessageBox;
import org.opendds.modeling.diagram.dcpslib.edit.commands.TopicTypeReferCommand;
import org.opendds.modeling.diagram.dcpslib.providers.OpenDDSDcpsLibElementTypes;
import org.opendds.modeling.model.types.Struct;
import org.opendds.modeling.model.types.TypesPackage;
import org.opendds.modeling.model.types.Type;

import com.ociweb.gmf.dialogs.ObjectsAcrossResourcesDialog;
import com.ociweb.gmf.edit.parts.AbstractRefSelectionAction;


/**
 * An action to add a Type-derived object specified by the user to a Topic.
 * Note that the Type in the case of OpenDDS will be a Struct, 
 * @generated NOT
 */
public class OpenDDSDcpsLibRefTypeAction<CompartEditPartType extends ListCompartmentEditPart> extends AbstractRefSelectionAction {

	private static final String packageNamePrefix = "org.opendds.modeling.diagram.dcpslib";
	private static final String typeName = "Struct";

	/**
	 * A unique portion of the class name for compartments used for shared datatypes.
	 * Used to distinguish between other compartments.
	 */
	private static final String CompartmentEditPartNameSubstring = "DataType";

	public OpenDDSDcpsLibRefTypeAction(Class referrerEditPart) {
		super(referrerEditPart, TopicTypeReferCommand.class);
	}

	@Override
	public void run(IAction action) {
		if (selectedElement == null) {
			return;
		}

		EObject domainElement = ((View) selectedElement.getModel()).getElement();
		EPackage typesPackage = EPackage.Registry.INSTANCE.getEPackage(TypesPackage.eNS_URI);
		EClass datatypeClass = (EClass) typesPackage.getEClassifier(typeName);
		Collection<EObject> reachableObjects =
			org.eclipse.emf.edit.provider.ItemPropertyDescriptor.getReachableObjectsOfType(domainElement, datatypeClass);

		// Filter out Structs not intended to be a DCPS data type
		List<Type> objsToRefCandidates = new ArrayList<Type>();
		for (EObject obj : reachableObjects) {
			Struct struct = (Struct) obj;
			if (struct.isIsDcpsDataType()) {
				objsToRefCandidates.add((Type) obj);
			}
		}

		if (objsToRefCandidates.size() == 0) {
			MessageBox message = new MessageBox(workbenchPart.getSite().getShell(), SWT.OK);
			message.setMessage("No " + typeName + "s with isDcpsDataType = true are available to select from.");
			message.open();
			return;
		}

		Collection<Type> objsToRef = getDatatypesToReferTo(objsToRefCandidates);

		addReferences(domainElement, objsToRef);

	}

	private List<Type> getDatatypesToReferTo(
			List<Type> objsToRefCandidates) {

		ObjectsAcrossResourcesDialog <Type> selectDialog =
			new  ObjectsAcrossResourcesDialog(typeName + " Selection", workbenchPart.getSite().getShell(), objsToRefCandidates, new StructLabeler());
		selectDialog.onlySingleItemSelectable();
		selectDialog.open();
		List<Type> datatypes = new ArrayList();
		for (Type datatype: selectDialog.getObjectsSelected()) {
			datatypes.add(datatype);
		}

		return datatypes;
	}

	@Override
	protected String getCompartmentEditPartNameSubstring() {
		return CompartmentEditPartNameSubstring;
	}

	@Override
	protected IElementType getElementType(int visualId) {
		return OpenDDSDcpsLibElementTypes.getElementType(visualId);
	}

	@Override
	protected Class getRefererrEditPartClass(EObject obj) {
		Class<?> clazz = null;
		String datatypeName = obj.eClass().getName();
		String className = packageNamePrefix + ".edit.parts." + capitialize(datatypeName) + "EditPart";
		try {
			clazz = Class.forName(className);
		} catch (ClassNotFoundException e) {
			e.printStackTrace();
		}
		return clazz;
	}

}
