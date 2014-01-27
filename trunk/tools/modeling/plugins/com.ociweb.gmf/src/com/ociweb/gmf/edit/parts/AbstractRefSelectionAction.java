package com.ociweb.gmf.edit.parts;

import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.util.Collection;
import java.util.List;

import org.eclipse.emf.ecore.EObject;
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
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.IObjectActionDelegate;
import org.eclipse.ui.IWorkbenchPart;

import com.ociweb.gmf.edit.commands.AddReferenceCommand;


/**
 * An abstract class for an action to add non-containment references specified by the user to an EObject.
 */
abstract public class AbstractRefSelectionAction<
CompartEditPartType extends ListCompartmentEditPart,
AddRefCommand extends AddReferenceCommand> implements IObjectActionDelegate {

	private final Class<AddRefCommand> addRefCommandClass;
	protected ShapeNodeEditPart selectedElement;

	protected final Class<?> referrerEditPart;

	protected IWorkbenchPart workbenchPart;

	public AbstractRefSelectionAction(
			Class<?> referrerEditPart,
			Class<AddRefCommand> addRefCommandClass) {
		super();
		this.referrerEditPart = referrerEditPart;
		this.addRefCommandClass = addRefCommandClass;
	}

	@Override
	abstract public void run(IAction action);

	/**
	 * Get a unique portion of the class name for compartments used for reference(s)
	 * to distinguish it from other compartments.
	 */
	abstract protected String getCompartmentEditPartNameSubstring();

	abstract protected Class<?> getRefererrEditPartClass(EObject obj);

	@SuppressWarnings("unchecked")
	protected <Type extends EObject> void addReferences(EObject domainElement,
			Collection<Type> objsToRef) {
		for (EObject obj: objsToRef) {
			// Get the compartment edit part that will contain the reference.
			List children = selectedElement.getChildren();
			CompartEditPartType compartmentEditPart = null;
			for (Object child: children) {
				if (child instanceof ListCompartmentEditPart  &&
						child.getClass().getName().contains(getCompartmentEditPartNameSubstring())) {
					compartmentEditPart = (CompartEditPartType) child;
					break;
				}
			}
			if (compartmentEditPart == null) {
				continue;
			}

			CompoundCommand cc = new CompoundCommand();

			// Create Request/Command to add Type reference to referrer
			IElementType elementType = getElementType(obj);
			CreateElementRequest createElementRequest = new CreateElementRequest(domainElement, elementType);
			createElementRequest.setNewElement(obj);
			try {
				Constructor ctor = addRefCommandClass.getDeclaredConstructor(CreateElementRequest.class);
				EditElementCommand referCommand = (AddReferenceCommand) ctor.newInstance(createElementRequest);
				cc.add(new ICommandProxy(referCommand));
			} catch (Exception e) {
				e.printStackTrace();
			}

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

	/**
	 * Gets the IElementType for a EObject in a rather roundabout way.
	 * Is there a more direct way to do this?
	 */
	private IElementType getElementType(EObject obj) {
		IElementType elementType = null;


		try {
			Class<?> objEditPartClass = getRefererrEditPartClass(obj);
			Field refVisualId = objEditPartClass.getField("VISUAL_ID");
			int visualId = refVisualId.getInt(null);
			elementType = getElementType(visualId);
		} catch (Exception e) {
			e.printStackTrace();
		}
		return elementType;
	}

	abstract protected IElementType getElementType(int visualId);

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

	protected static String capitialize(String s) {
		return s.substring(0, 1).toUpperCase() + s.substring(1);
	}

	@Override
	public void selectionChanged(IAction action, ISelection selection) {
		selectedElement = null;
		if (selection instanceof IStructuredSelection) {
			IStructuredSelection structuredSelection = (IStructuredSelection) selection;
			if (referrerEditPart.isInstance(structuredSelection.getFirstElement())) {
				selectedElement = (ShapeNodeEditPart) structuredSelection
				.getFirstElement();
			}
		}
	}

	@Override
	public void setActivePart(IAction action, IWorkbenchPart targetPart) {
		workbenchPart = targetPart;
	}

}
