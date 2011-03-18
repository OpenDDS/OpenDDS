package org.opendds.modeling.diagram.dcpslib.part;

import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.List;

import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EPackage;
import org.eclipse.gef.EditPart;
import org.eclipse.gef.EditPartViewer;
import org.eclipse.gef.GraphicalEditPart;
import org.eclipse.gef.Request;
import org.eclipse.gef.commands.Command;
import org.eclipse.gmf.runtime.diagram.ui.editparts.ListCompartmentEditPart;
import org.eclipse.gmf.runtime.diagram.ui.editparts.ShapeNodeEditPart;
import org.eclipse.gmf.runtime.diagram.ui.requests.CreateViewRequest;
import org.eclipse.gmf.runtime.diagram.ui.requests.CreateViewRequestFactory;
import org.eclipse.gmf.runtime.notation.View;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.IObjectActionDelegate;
import org.eclipse.ui.IWorkbenchPart;
import org.eclipse.ui.dialogs.ListSelectionDialog;
import org.opendds.modeling.diagram.dcpslib.providers.OpenDDSDcpsLibElementTypes;

import com.ociweb.emf.util.ClassesFinder;
import com.ociweb.emf.util.ReferencesFinder;

/**
 * An action to automatically create a QosPolicy-derived object and add as a reference
 * to a domain entity. Based on code given in the Eclipse Modeling Project book,
 * section "Adding a Subtopic Action" on page 90. The code was extended to be
 * generic so that it can be used with different domain entity types.
 * The term "referrer" refers to the object (domain entities in this
 * case) that has a reference to the QoS Policy.
 *
 * @generated NOT
 *
 */
public class OpenDDSDcpsLibCreateQosPolicyAction<CompartmentEditPartType extends ListCompartmentEditPart>
implements IObjectActionDelegate {

	/**
	 * A unique portion of the class name for compartments used for custom policies.
	 * Used to distinguish between compartments used for shared policies.
	 */
	private static final String CompartmentEditPartNameSubstring = "QoSPoliciesCustom";

	public OpenDDSDcpsLibCreateQosPolicyAction(Class referrerEditPart) {
		super();
		featureReferrerEditPart = referrerEditPart;
	}

	private ShapeNodeEditPart selectedElement;

	private final Class featureReferrerEditPart;

	private IWorkbenchPart workbenchPart;

	public void run(IAction action) {

		Collection<String> features = null;

		features = identifyPolicies(selectedElement);

		ListSelectionDialog listDialog = new ListSelectionDialog(workbenchPart.getSite().getShell(),
				features.toArray(),
				new ArrayContentProvider(), new LabelProvider(),
		"Select QoS Policies to include");
		listDialog.open();
		Object[] selection = listDialog.getResult();

		if (selectedElement == null) {
			return;
		}

		for (Object s : selection) {
			String policyName = s.toString();

			String policyNameCap = policyName.substring(0, 1).toUpperCase() + policyName.substring(1);
			String policyEditPartClassName = "org.opendds.modeling.diagram.dcpslib.edit.parts." + policyNameCap + "EditPart";
			try {

				Class policyEditPartClass = Class.forName(policyEditPartClassName);
				Field policyVisualId = policyEditPartClass.getField("VISUAL_ID");

				// Create policy view
				CreateViewRequest policyCreateViewRequest = CreateViewRequestFactory.getCreateShapeRequest(
						OpenDDSDcpsLibElementTypes.getElementType(policyVisualId.getInt(null)),
						selectedElement.getDiagramPreferencesHint());
				// Get the compartment edit part that will contain the policy
				List children = selectedElement.getChildren();
				CompartmentEditPartType compartmentEditPart = null;
				for (Object obj: children) {
					if (obj instanceof ListCompartmentEditPart &&
							obj.getClass().getName().contains(CompartmentEditPartNameSubstring)) {
						compartmentEditPart = (CompartmentEditPartType) obj;
						break;
					}
				}
				if (compartmentEditPart != null) {
					Command policyCreateViewCommand = compartmentEditPart.getCommand(policyCreateViewRequest);
					selectedElement.getDiagramEditDomain().getDiagramCommandStack()
					.execute(policyCreateViewCommand);
				}

				updateFigureView(compartmentEditPart);
			} catch (Throwable e) {
				e.printStackTrace();
			}
		}

	}

	/**
	 * @param editPart The Edit Part corresponding to the referrer to add Features to
	 * @return An alphabetized collection of names of derived Feature types that can be added.
	 */
	private Collection<String> identifyPolicies(ShapeNodeEditPart editPart) {
		List<String> policies = new ArrayList<String>();
		EObject domainElement = ((View) editPart.getModel()).getElement();
		EClass domainElementClass = domainElement.eClass();
		EPackage qosPackage = EPackage.Registry.INSTANCE.getEPackage(org.opendds.modeling.model.qos.QoSPackage.eNS_URI);
		EClass policyClass = (EClass) qosPackage.getEClassifier("QosPolicy");
		EList<EClass> refs = ReferencesFinder.findDerivedFrom(policyClass, domainElementClass);
		EPackage openddsPackage = EPackage.Registry.INSTANCE.getEPackage(org.opendds.modeling.model.opendds.OpenDDSPackage.eNS_URI);
		for (EClass ref : refs) {
			// The reference is the non-stereotype form of the policy. Since the edit parts are based
			// on the stereotypes we need to find the policy that is derived from this one.
			EList<EClass> derived = ClassesFinder.findDerived(ref, openddsPackage);
			assert derived.size() == 1;
			if (ReferencesFinder.isReferenceCandidate(ref, domainElement)) {
				policies.add(derived.get(0).getName());
			}
		}
		Collections.sort(policies);
		return policies;
	}

	private void updateFigureView(GraphicalEditPart containerEditPart) {
		final EditPartViewer viewer = selectedElement.getViewer();
		assert containerEditPart.getChildren() != null;
		assert containerEditPart.getChildren().size() > 0;
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

	public void selectionChanged(IAction action, ISelection selection) {
		selectedElement = null;
		if (selection instanceof IStructuredSelection) {
			IStructuredSelection structuredSelection = (IStructuredSelection) selection;
			if (featureReferrerEditPart.isInstance(structuredSelection.getFirstElement())) {
				selectedElement = (ShapeNodeEditPart) structuredSelection
				.getFirstElement();
			}
		}
	}

	public void setActivePart(IAction action, IWorkbenchPart targetPart) {
		workbenchPart = targetPart;
	}


}
