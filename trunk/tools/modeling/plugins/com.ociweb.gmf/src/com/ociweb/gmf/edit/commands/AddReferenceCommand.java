package com.ociweb.gmf.edit.commands;


import java.util.Collections;
import java.util.Map;

import org.eclipse.core.commands.ExecutionException;
import org.eclipse.core.runtime.IAdaptable;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.gmf.runtime.common.core.command.CommandResult;
import org.eclipse.gmf.runtime.common.core.command.ICommand;
import org.eclipse.gmf.runtime.emf.type.core.IElementType;
import org.eclipse.gmf.runtime.emf.type.core.commands.EditElementCommand;
import org.eclipse.gmf.runtime.emf.type.core.requests.ConfigureRequest;
import org.eclipse.gmf.runtime.emf.type.core.requests.CreateElementRequest;
import org.eclipse.gmf.runtime.emf.type.core.requests.IEditCommandRequest;
import org.eclipse.gmf.runtime.notation.View;

/**
 * An abstract GMF command to add a reference to a EClass.
 * This is needed because GMF 2.2.2 does not support parent figures with
 * child figures whose associated domain elements are non-containment references
 * in the parent element associated with the parent figure.
 */
abstract public class AddReferenceCommand extends EditElementCommand {

	private final Map<String, String> classNameToRefName;

	public AddReferenceCommand(CreateElementRequest req,
			Map<String, String> classNameToRefName) {
		super(req.getLabel(), null, req);
		this.classNameToRefName = Collections.unmodifiableMap(classNameToRefName);
	}

	@Override
	public boolean canExecute() {
		return true;
	}

	@Override
	protected CommandResult doExecuteWithResult(IProgressMonitor monitor,
			IAdaptable info) throws ExecutionException {

		IEditCommandRequest request = getRequest();
		assert request instanceof CreateElementRequest;

		EObject newElement = ((CreateElementRequest) request).getNewElement();

		// Add to referrer
		EObject referrer = getElementToEdit();
		EClass referrerClass = referrer.eClass();
		String newElementClassName = newElement.eClass().getName();
		String refName = classNameToRefName.get(newElementClassName);
		EStructuralFeature structuralFeature = referrerClass.getEStructuralFeature(refName);
		referrer.eSet(structuralFeature, newElement);

		doConfigure(newElement, monitor, info);

		((CreateElementRequest) getRequest()).setNewElement(newElement);
		return CommandResult.newOKCommandResult(newElement);
	}

	/**
	 * Based on GMF 2.2.2 generated code take from CreateCommand.
	 * If generated code changes then this would have to similarly change.
	 */
	@Override
	protected EObject getElementToEdit() {
		EObject container = ((CreateElementRequest) getRequest())
				.getContainer();
		if (container instanceof View) {
			container = ((View) container).getElement();
		}
		return container;
	}

	/**
	 * Based on GMF 2.2.2 generated code take from CreateCommand.
	 * If generated code changes then this would have to similarly change.
	 */
	protected void doConfigure(EObject newElement, IProgressMonitor monitor,
			IAdaptable info) throws ExecutionException {
		IElementType elementType = ((CreateElementRequest) getRequest())
				.getElementType();
		ConfigureRequest configureRequest = new ConfigureRequest(
				getEditingDomain(), newElement, elementType);
		configureRequest.setClientContext(((CreateElementRequest) getRequest())
				.getClientContext());
		configureRequest.addParameters(getRequest().getParameters());
		ICommand configureCommand = elementType
				.getEditCommand(configureRequest);
		if (configureCommand != null && configureCommand.canExecute()) {
			configureCommand.execute(monitor, info);
		}
	}

}
