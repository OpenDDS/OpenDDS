package org.opendds.modeling.model.opendds.diagram.dcpslib.part;

import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;

import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EPackage;
import org.eclipse.gef.EditPart;
import org.eclipse.gef.EditPartViewer;
import org.eclipse.gef.GraphicalEditPart;
import org.eclipse.gef.Request;
import org.eclipse.gef.commands.Command;
import org.eclipse.gef.commands.CompoundCommand;
import org.eclipse.gmf.runtime.diagram.core.preferences.PreferencesHint;
import org.eclipse.gmf.runtime.diagram.ui.commands.ICommandProxy;
import org.eclipse.gmf.runtime.diagram.ui.editparts.ListCompartmentEditPart;
import org.eclipse.gmf.runtime.diagram.ui.editparts.ShapeNodeEditPart;
import org.eclipse.gmf.runtime.diagram.ui.requests.CreateViewRequest;
import org.eclipse.gmf.runtime.emf.core.util.EObjectAdapter;
import org.eclipse.gmf.runtime.emf.type.core.IElementType;
import org.eclipse.gmf.runtime.emf.type.core.commands.EditElementCommand;
import org.eclipse.gmf.runtime.emf.type.core.requests.CreateElementRequest;
import org.eclipse.gmf.runtime.notation.Node;
import org.eclipse.gmf.runtime.notation.View;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.MessageBox;
import org.eclipse.ui.IObjectActionDelegate;
import org.eclipse.ui.IWorkbenchPart;
import org.opendds.modeling.model.opendds.OpenDDSPackage;
import org.opendds.modeling.model.opendds.diagram.dcpslib.edit.commands.QosPolicyReferCommand;
import org.opendds.modeling.model.opendds.diagram.dcpslib.providers.OpenDDSDcpsLibElementTypes;
import org.opendds.modeling.model.qos.QoSPackage;
import org.opendds.modeling.model.qos.QosPolicy;

import com.ociweb.emf.util.ReferencesFinder;
import com.ociweb.gmf.dialogs.ObjectsAcrossResourcesDialog;


/**
 * An action to automatically add a QosPolicy-derived object as a non-containment reference
 * to a Domain Entity. This is in contrast to OpenDDSDcpsLibCreateQosPolicyAction, where the
 * Domain Entity "owns" the policy (even though from an Ecore perspective it's a non-containment
 * reference).
 *
 * @see OpenDDSDcpsLibCreateQosPolicyAction
 * @generated NOT
 */
public class OpenDDSDcpsLibRefQosPolicyAction<CompartEditPartType extends ListCompartmentEditPart> implements IObjectActionDelegate {

	private static final String PackageNamePrefix = "org.opendds.modeling.model.opendds.diagram.dcpslib";

	/**
	 * A unique portion of the class name for compartments used for shared policies.
	 * Used to distinguish between compartments used for custom policies.
	 */
	private static final String CompartmentEditPartNameSubstring = "QoSPoliciesShared";

	public OpenDDSDcpsLibRefQosPolicyAction(Class referrerEditPart) {
		super();
		policyReferrerEditPart = referrerEditPart;
	}
	private ShapeNodeEditPart selectedElement;

	private final Class policyReferrerEditPart;

	private IWorkbenchPart workbenchPart;

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
			if (ReferencesFinder.isReferenceCandidate(obj, domainElement)) {
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

		for (EObject obj: objsToRef) {
			// Get the compartment edit part that will contain the policy
			List children = selectedElement.getChildren();
			CompartEditPartType compartmentEditPart = null;
			for (Object child: children) {
				if (child instanceof ListCompartmentEditPart  &&
						child.getClass().getName().contains(CompartmentEditPartNameSubstring)) {
					compartmentEditPart = (CompartEditPartType) child;
					break;
				}
			}
			if (compartmentEditPart == null) {
				continue;
			}

			CompoundCommand cc = new CompoundCommand();

			// Create Request/Command to add QosPolicy reference to Domain Entity
			IElementType elementType = getElementType(obj);
			CreateElementRequest createElementRequest = new CreateElementRequest(domainElement, elementType);
			createElementRequest.setNewElement(obj);
			EditElementCommand policyReferCommand = new QosPolicyReferCommand(createElementRequest);
			cc.add(new ICommandProxy(policyReferCommand));

			// Create and execute command to create the view
			EObjectAdapter oa = new EObjectAdapter(obj);
			PreferencesHint hint = selectedElement.getDiagramPreferencesHint();
			org.eclipse.gmf.runtime.diagram.ui.requests.CreateViewRequest.ViewDescriptor viewDescriptor =
				new CreateViewRequest.ViewDescriptor(oa, Node.class, null, hint);
			CreateViewRequest createViewRequest = new CreateViewRequest(viewDescriptor);
			// Returns a CompoundCommand which includes a CreateCommand.
			Command createCommand = compartmentEditPart.getCommand(createViewRequest);
			assert createCommand.canExecute();

			cc.add(createCommand);

			selectedElement.getDiagramEditDomain().getDiagramCommandStack()
			.execute(cc);

			updateFigureView(selectedElement);
		}

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

	/**
	 * Gets the IElementType for a EObject in a rather roundabout way.
	 * Is there a more direct way to do this?
	 */
	private IElementType getElementType(EObject obj) {
		IElementType elementType = null;

		String policyName = obj.eClass().getName();
		String className = PackageNamePrefix + ".edit.parts." + capitialize(policyName) + "EditPart";

		try {
			Class objEditPartClass = Class.forName(className);
			Field policyVisualId = objEditPartClass.getField("VISUAL_ID");
			int visualId = policyVisualId.getInt(null);
			elementType = OpenDDSDcpsLibElementTypes.getElementType(visualId);
		} catch (Exception e) {
			e.printStackTrace();
		}
		return elementType;
	}

	private void updateFigureView(GraphicalEditPart containerEditPart) {
		final EditPartViewer viewer = selectedElement.getViewer();
		final EditPart ep = (EditPart) containerEditPart.getChildren().get(
				containerEditPart.getChildren().size() - 1);
		if (ep != null) {
			viewer.setSelection(new StructuredSelection(ep));
			viewer.reveal(ep);
			Display.getCurrent().syncExec(new Runnable() {
				public void run() {
					Request der = new Request(org.eclipse.gef.RequestConstants.REQ_DIRECT_EDIT);
					ep.performRequest(der);
				}
			});
		}
	}

	private static String capitialize(String s) {
		return s.substring(0, 1).toUpperCase() + s.substring(1);
	}

	public void selectionChanged(IAction action, ISelection selection) {
		selectedElement = null;
		if (selection instanceof IStructuredSelection) {
			IStructuredSelection structuredSelection = (IStructuredSelection) selection;
			if (policyReferrerEditPart.isInstance(structuredSelection.getFirstElement())) {
				selectedElement = (ShapeNodeEditPart) structuredSelection
						.getFirstElement();
			}
		}
	}

	public void setActivePart(IAction action, IWorkbenchPart targetPart) {
		workbenchPart = targetPart;
	}

}
