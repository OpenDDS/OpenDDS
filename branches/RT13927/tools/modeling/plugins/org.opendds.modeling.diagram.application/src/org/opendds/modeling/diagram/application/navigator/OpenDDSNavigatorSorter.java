package org.opendds.modeling.diagram.application.navigator;

import org.eclipse.jface.viewers.ViewerSorter;
import org.opendds.modeling.diagram.application.part.OpenDDSVisualIDRegistry;

/**
 * @generated
 */
public class OpenDDSNavigatorSorter extends ViewerSorter {

	/**
	 * @generated
	 */
	private static final int GROUP_CATEGORY = 4003;

	/**
	 * @generated
	 */
	@Override
	public int category(Object element) {
		if (element instanceof OpenDDSNavigatorItem) {
			OpenDDSNavigatorItem item = (OpenDDSNavigatorItem) element;
			return OpenDDSVisualIDRegistry.getVisualID(item.getView());
		}
		return GROUP_CATEGORY;
	}

}
