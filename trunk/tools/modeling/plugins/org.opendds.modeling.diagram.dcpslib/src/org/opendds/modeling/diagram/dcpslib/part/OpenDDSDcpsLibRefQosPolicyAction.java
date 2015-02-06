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
import org.opendds.modeling.model.opendds.OpenDDSPackage;
import org.opendds.modeling.diagram.dcpslib.edit.commands.QosPolicyReferCommand;
import org.opendds.modeling.diagram.dcpslib.providers.OpenDDSDcpsLibElementTypes;
import org.opendds.modeling.model.qos.QoSPackage;
import org.opendds.modeling.model.qos.QosPolicy;
import com.ociweb.emf.util.ReferencesFinder;
import com.ociweb.gmf.dialogs.ObjectsAcrossResourcesDialog;
import com.ociweb.gmf.edit.parts.AbstractRefSelectionAction;


/**
 * An action to add a QosPolicy-derived object specified by the user to a DCPS domain entity.
 * @generated NOT
 */
public class OpenDDSDcpsLibRefQosPolicyAction<CompartEditPartType extends ListCompartmentEditPart> extends AbstractRefSelectionAction {

	private static final String packageNamePrefix = "org.opendds.modeling.diagram.dcpslib";

	/**
	 * A unique portion of the class name for compartments used for shared policies.
	 * Used to distinguish between compartments used for custom policies.
	 */
	private static final String CompartmentEditPartNameSubstring = "QoSPoliciesShared";

	public OpenDDSDcpsLibRefQosPolicyAction(Class referrerEditPart) {
		super(referrerEditPart, QosPolicyReferCommand.class);
	}

	@Override
	public void run(IAction action) {

		if (selectedElement == null) {
			return;
		}

		EObject domainElement = ((View) selectedElement.getModel()).getElement();
		EPackage qosPackage = EPackage.Registry.INSTANCE.getEPackage(QoSPackage.eNS_URI);
		EClass policyClass = (EClass) qosPackage.getEClassifier("QosPolicy");
		Collection<EObject> reachableObjects =
			org.eclipse.emf.edit.provider.ItemPropertyDescriptor.getReachableObjectsOfType(domainElement, policyClass);

		// ItemPropertyDescriptor.getReachableObjectsOfType() returns all QosPolicy derived objects instead of
		// just those that can be associated with domainElement. So filter out objects that cannot be associated.
		List<QosPolicy> objsToRefCandidates = new ArrayList<QosPolicy>();
		for (EObject obj : reachableObjects) {
			if (ReferencesFinder.isReferenceCandidate(obj.eClass(), domainElement)) {
				objsToRefCandidates.add((QosPolicy) obj);
			}
		}

		// Filter out policies that are custom (owned by a DcpsLib)
		EPackage dcpsLibsPackage = EPackage.Registry.INSTANCE.getEPackage(OpenDDSPackage.eNS_URI);
		EClass dcpsLibClass = (EClass) dcpsLibsPackage.getEClassifier("DcpsLib");
		List<QosPolicy> objsToRefCandidatesInOtherResources = new ArrayList<QosPolicy>();
		for (QosPolicy obj : objsToRefCandidates) {
			EObject policyContainer = obj.eContainer();
			EClass policyContainerClass = policyContainer.eClass();
			if (! policyContainerClass.isSuperTypeOf(dcpsLibClass)) {
				objsToRefCandidatesInOtherResources.add(obj);
			}
		}

		if (objsToRefCandidatesInOtherResources.size() == 0) {
			MessageBox message = new MessageBox(workbenchPart.getSite().getShell(), SWT.OK);
			message.setMessage("No appropriate references from other resources are available to select from.");
			message.open();
			return;
		}

		Collection<QosPolicy> objsToRef = getPoliciesToReferTo(objsToRefCandidatesInOtherResources);

		addReferences(domainElement, objsToRef);

	}

	private List<QosPolicy> getPoliciesToReferTo(
			List<QosPolicy> objsToRefCandidates) {

		ObjectsAcrossResourcesDialog <QosPolicy> selectDialog =
			new  ObjectsAcrossResourcesDialog("Shared QoS Policy Selection", workbenchPart.getSite().getShell(), objsToRefCandidates, new QosPolicyLabeler());
		selectDialog.open();
		List<QosPolicy> policies = new ArrayList();
		for (QosPolicy policy: selectDialog.getObjectsSelected()) {
			policies.add(policy);
		}

		return policies;
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
