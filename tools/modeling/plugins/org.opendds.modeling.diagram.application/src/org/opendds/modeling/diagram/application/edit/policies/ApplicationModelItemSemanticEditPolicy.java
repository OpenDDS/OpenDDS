package org.opendds.modeling.diagram.application.edit.policies;

import org.eclipse.emf.transaction.TransactionalEditingDomain;
import org.eclipse.gef.commands.Command;
import org.eclipse.gmf.runtime.diagram.ui.editparts.IGraphicalEditPart;
import org.eclipse.gmf.runtime.emf.commands.core.commands.DuplicateEObjectsCommand;
import org.eclipse.gmf.runtime.emf.type.core.requests.CreateElementRequest;
import org.eclipse.gmf.runtime.emf.type.core.requests.DuplicateElementsRequest;
import org.opendds.modeling.diagram.application.edit.commands.ApplicationTargetCreateCommand;
import org.opendds.modeling.diagram.application.providers.OpenDDSElementTypes;

/**
 * @generated
 */
public class ApplicationModelItemSemanticEditPolicy extends
		OpenDDSBaseItemSemanticEditPolicy {

	/**
	 * @generated
	 */
	public ApplicationModelItemSemanticEditPolicy() {
		super(OpenDDSElementTypes.ApplicationModel_1000);
	}

	/**
	 * @generated
	 */
	@Override
	protected Command getCreateCommand(CreateElementRequest req) {
		if (OpenDDSElementTypes.ApplicationTarget_2001 == req.getElementType()) {
			return getGEFWrapper(new ApplicationTargetCreateCommand(req));
		}
		return super.getCreateCommand(req);
	}

	/**
	 * @generated
	 */
	@Override
	protected Command getDuplicateCommand(DuplicateElementsRequest req) {
		TransactionalEditingDomain editingDomain = ((IGraphicalEditPart) getHost())
				.getEditingDomain();
		return getGEFWrapper(new DuplicateAnythingCommand(editingDomain, req));
	}

	/**
	 * @generated
	 */
	private static class DuplicateAnythingCommand extends
			DuplicateEObjectsCommand {

		/**
		 * @generated
		 */
		public DuplicateAnythingCommand(
				TransactionalEditingDomain editingDomain,
				DuplicateElementsRequest req) {
			super(editingDomain, req.getLabel(), req
					.getElementsToBeDuplicated(), req
					.getAllDuplicatedElementsMap());
		}

	}

}
