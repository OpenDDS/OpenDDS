/**
 *
 */
package org.opendds.modeling.common.gmf;

import org.eclipse.emf.ecore.EObject;
import org.eclipse.gmf.runtime.diagram.ui.editparts.GraphicalEditPart;
import org.eclipse.gmf.runtime.notation.View;
import org.eclipse.jface.viewers.IFilter;
import org.opendds.modeling.model.core.Element;

/**
 * Only select EObjects that inherit from org.opendds.modeling.model.core.Element, or EditParts
 * related to these EObjects.
 * Used for the Comment Property tab.
 */
public class ElementCommentFilter implements IFilter {

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IFilter#select(java.lang.Object)
	 */
	@Override
	public boolean select(Object toTest) {

		if (toTest instanceof Element) {
			return true;
		}

        if (toTest instanceof GraphicalEditPart) {
        	GraphicalEditPart editPart = (GraphicalEditPart) toTest;
        	Object model = editPart.getModel();
        	if (model instanceof View) {
        		EObject element = ((View) model).getElement();
        		if (BasicTypeIdentifier.isBasic(element.eClass())) {
        			return false;
        		}
        		if (element instanceof Element) {
        			return true;
        		}
        	}
        }
		return false;
	}

}
