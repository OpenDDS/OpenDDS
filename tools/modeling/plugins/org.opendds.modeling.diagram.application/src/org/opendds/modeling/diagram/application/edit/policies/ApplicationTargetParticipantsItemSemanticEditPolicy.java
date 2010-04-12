package org.opendds.modeling.diagram.application.edit.policies;

import org.eclipse.gef.commands.Command;
import org.eclipse.gmf.runtime.emf.type.core.commands.DestroyReferenceCommand;
import org.eclipse.gmf.runtime.emf.type.core.requests.DestroyReferenceRequest;
import org.opendds.modeling.diagram.application.providers.OpenDDSElementTypes;

/**
 * @generated
 */
public class ApplicationTargetParticipantsItemSemanticEditPolicy extends
		OpenDDSBaseItemSemanticEditPolicy {

	/**
	 * @generated
	 */
	public ApplicationTargetParticipantsItemSemanticEditPolicy() {
		super(OpenDDSElementTypes.ApplicationTargetParticipants_4001);
	}

	/**
	 * @generated
	 */
	@Override
	protected Command getDestroyReferenceCommand(DestroyReferenceRequest req) {
		return getGEFWrapper(new DestroyReferenceCommand(req));
	}

}
