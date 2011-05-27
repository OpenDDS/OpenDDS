package com.ociweb.gmf.edit.commands;

import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.transaction.TransactionalEditingDomain;
import org.eclipse.gef.EditPart;
import org.eclipse.gmf.runtime.emf.type.core.commands.DestroyReferenceCommand;
import org.eclipse.gmf.runtime.emf.type.core.requests.DestroyElementRequest;
import org.eclipse.gmf.runtime.emf.type.core.requests.DestroyReferenceRequest;
import org.eclipse.gmf.runtime.notation.BasicDecorationNode;

public class RequestToCommandConverter {

	public static DestroyReferenceCommand destroyElementRequestToDestroyReferenceCommand(
			DestroyElementRequest req, EditPart host, TransactionalEditingDomain editingDomain) {
		EditPart parentEditPart = host.getParent();
		EObject parentElement = ((BasicDecorationNode) parentEditPart.getModel()).getElement();
		DestroyReferenceRequest destroyRefReq =
			new DestroyReferenceRequest(editingDomain, parentElement, null, req.getElementToDestroy(), false);
		DestroyReferenceCommand destroyRefCmd = new DestroyReferenceCommand(destroyRefReq);
		return destroyRefCmd;
	}

}
