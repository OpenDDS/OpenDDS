package org.opendds.modeling.diagram.application.navigator;

import org.eclipse.gmf.runtime.emf.type.core.IElementType;
import org.eclipse.gmf.runtime.notation.View;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.jface.resource.ImageRegistry;
import org.eclipse.jface.viewers.ITreePathLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.TreePath;
import org.eclipse.jface.viewers.ViewerLabel;
import org.eclipse.swt.graphics.Image;
import org.eclipse.ui.IMemento;
import org.eclipse.ui.navigator.ICommonContentExtensionSite;
import org.eclipse.ui.navigator.ICommonLabelProvider;
import org.opendds.modeling.diagram.application.edit.parts.ApplicationModelEditPart;
import org.opendds.modeling.diagram.application.edit.parts.ApplicationTargetEditPart;
import org.opendds.modeling.diagram.application.edit.parts.ApplicationTargetParticipantsEditPart;
import org.opendds.modeling.diagram.application.part.OpenDDSDiagramEditorPlugin;
import org.opendds.modeling.diagram.application.part.OpenDDSVisualIDRegistry;
import org.opendds.modeling.diagram.application.providers.OpenDDSElementTypes;

/**
 * @generated
 */
public class OpenDDSNavigatorLabelProvider extends LabelProvider implements
		ICommonLabelProvider, ITreePathLabelProvider {

	/**
	 * @generated
	 */
	static {
		OpenDDSDiagramEditorPlugin
				.getInstance()
				.getImageRegistry()
				.put(
						"Navigator?UnknownElement", ImageDescriptor.getMissingImageDescriptor()); //$NON-NLS-1$
		OpenDDSDiagramEditorPlugin
				.getInstance()
				.getImageRegistry()
				.put(
						"Navigator?ImageNotFound", ImageDescriptor.getMissingImageDescriptor()); //$NON-NLS-1$
	}

	/**
	 * @generated
	 */
	public void updateLabel(ViewerLabel label, TreePath elementPath) {
		Object element = elementPath.getLastSegment();
		if (element instanceof OpenDDSNavigatorItem
				&& !isOwnView(((OpenDDSNavigatorItem) element).getView())) {
			return;
		}
		label.setText(getText(element));
		label.setImage(getImage(element));
	}

	/**
	 * @generated
	 */
	@Override
	public Image getImage(Object element) {
		if (element instanceof OpenDDSNavigatorGroup) {
			OpenDDSNavigatorGroup group = (OpenDDSNavigatorGroup) element;
			return OpenDDSDiagramEditorPlugin.getInstance().getBundledImage(
					group.getIcon());
		}

		if (element instanceof OpenDDSNavigatorItem) {
			OpenDDSNavigatorItem navigatorItem = (OpenDDSNavigatorItem) element;
			if (!isOwnView(navigatorItem.getView())) {
				return super.getImage(element);
			}
			return getImage(navigatorItem.getView());
		}

		return super.getImage(element);
	}

	/**
	 * @generated
	 */
	public Image getImage(View view) {
		switch (OpenDDSVisualIDRegistry.getVisualID(view)) {
		case ApplicationModelEditPart.VISUAL_ID:
			return getImage(
					"Navigator?Diagram?http://www.opendds.org/schemas/modeling/OpenDDS?ApplicationModel", OpenDDSElementTypes.ApplicationModel_1000); //$NON-NLS-1$
		case ApplicationTargetEditPart.VISUAL_ID:
			return getImage(
					"Navigator?TopLevelNode?http://www.opendds.org/schemas/modeling/OpenDDS?ApplicationTarget", OpenDDSElementTypes.ApplicationTarget_2001); //$NON-NLS-1$
		case ApplicationTargetParticipantsEditPart.VISUAL_ID:
			return getImage(
					"Navigator?Link?http://www.opendds.org/schemas/modeling/OpenDDS?ApplicationTarget?participants", OpenDDSElementTypes.ApplicationTargetParticipants_4001); //$NON-NLS-1$
		}
		return getImage("Navigator?UnknownElement", null); //$NON-NLS-1$
	}

	/**
	 * @generated
	 */
	private Image getImage(String key, IElementType elementType) {
		ImageRegistry imageRegistry = OpenDDSDiagramEditorPlugin.getInstance()
				.getImageRegistry();
		Image image = imageRegistry.get(key);
		if (image == null && elementType != null
				&& OpenDDSElementTypes.isKnownElementType(elementType)) {
			image = OpenDDSElementTypes.getImage(elementType);
			imageRegistry.put(key, image);
		}

		if (image == null) {
			image = imageRegistry.get("Navigator?ImageNotFound"); //$NON-NLS-1$
			imageRegistry.put(key, image);
		}
		return image;
	}

	/**
	 * @generated
	 */
	@Override
	public String getText(Object element) {
		if (element instanceof OpenDDSNavigatorGroup) {
			OpenDDSNavigatorGroup group = (OpenDDSNavigatorGroup) element;
			return group.getGroupName();
		}

		if (element instanceof OpenDDSNavigatorItem) {
			OpenDDSNavigatorItem navigatorItem = (OpenDDSNavigatorItem) element;
			if (!isOwnView(navigatorItem.getView())) {
				return null;
			}
			return getText(navigatorItem.getView());
		}

		return super.getText(element);
	}

	/**
	 * @generated
	 */
	public String getText(View view) {
		if (view.getElement() != null && view.getElement().eIsProxy()) {
			return getUnresolvedDomainElementProxyText(view);
		}
		switch (OpenDDSVisualIDRegistry.getVisualID(view)) {
		case ApplicationModelEditPart.VISUAL_ID:
			return getApplicationModel_1000Text(view);
		case ApplicationTargetEditPart.VISUAL_ID:
			return getApplicationTarget_2001Text(view);
		case ApplicationTargetParticipantsEditPart.VISUAL_ID:
			return getApplicationTargetParticipants_4001Text(view);
		}
		return getUnknownElementText(view);
	}

	/**
	 * @generated
	 */
	private String getApplicationModel_1000Text(View view) {
		return ""; //$NON-NLS-1$
	}

	/**
	 * @generated
	 */
	private String getApplicationTarget_2001Text(View view) {
		OpenDDS.ApplicationTarget domainModelElement = (OpenDDS.ApplicationTarget) view
				.getElement();
		if (domainModelElement != null) {
			return String.valueOf(domainModelElement.getComponent_type());
		} else {
			OpenDDSDiagramEditorPlugin.getInstance().logError(
					"No domain element for view with visualID = " + 2001); //$NON-NLS-1$
			return ""; //$NON-NLS-1$
		}
	}

	/**
	 * @generated
	 */
	private String getApplicationTargetParticipants_4001Text(View view) {
		return ""; //$NON-NLS-1$
	}

	/**
	 * @generated
	 */
	private String getUnknownElementText(View view) {
		return "<UnknownElement Visual_ID = " + view.getType() + ">"; //$NON-NLS-1$  //$NON-NLS-2$
	}

	/**
	 * @generated
	 */
	private String getUnresolvedDomainElementProxyText(View view) {
		return "<Unresolved domain element Visual_ID = " + view.getType() + ">"; //$NON-NLS-1$  //$NON-NLS-2$
	}

	/**
	 * @generated
	 */
	public void init(ICommonContentExtensionSite aConfig) {
	}

	/**
	 * @generated
	 */
	public void restoreState(IMemento aMemento) {
	}

	/**
	 * @generated
	 */
	public void saveState(IMemento aMemento) {
	}

	/**
	 * @generated
	 */
	public String getDescription(Object anElement) {
		return null;
	}

	/**
	 * @generated
	 */
	private boolean isOwnView(View view) {
		return ApplicationModelEditPart.MODEL_ID.equals(OpenDDSVisualIDRegistry
				.getModelID(view));
	}

}
