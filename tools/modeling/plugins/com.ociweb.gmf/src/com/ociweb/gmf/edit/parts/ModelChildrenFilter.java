package com.ociweb.gmf.edit.parts;

import java.util.ArrayList;
import java.util.List;

import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.gmf.runtime.notation.View;

/**
 * Provides generic filtering for Model Children of EditParts to
 * restrict, for example, what shows up in Figure compartments.
 * @generated NOT
 *
 */
public class ModelChildrenFilter {

	/**
	 * Filter out model children that reside in a different Resource than the
	 * EditPart's model.
	 */
	@SuppressWarnings("unchecked")
	public static List filterChildrenInDifferentResource(View editPartModel, List children) {
		List childrenAll = new ArrayList(children);
		EObject element = editPartModel.getElement();
		Resource modelResource = element.eResource();
		for (Object child: childrenAll) {
			if (child instanceof View) {
				View childView = (View) child;
				EObject childElement = childView.getElement();
				Resource childResource = childElement.eResource();
				if (!modelResource.equals(childResource)) {
					children.remove(child);
				}
			}
		}
		return children;
	}
	
	/**
	 * Filter out model children that reside in a different Resource than the
	 * EditPart's model.
	 */
	@SuppressWarnings("unchecked")
	public static List filterChildrenInSameResource(View editPartModel, List children) {
		List childrenAll = new ArrayList(children);
		EObject element = editPartModel.getElement();
		Resource modelResource = element.eResource();
		for (Object child: childrenAll) {
			if (child instanceof View) {
				View childView = (View) child;
				EObject childElement = childView.getElement();
				Resource childResource = childElement.eResource();
				if (modelResource.equals(childResource)) {
					children.remove(child);
				}
			}
		}
		return children;
	}

	/**
	 * Filter out children whose owner is of a particular type.
	 */
	@SuppressWarnings("unchecked")
	public static List filterChildrenByOwner(EClass ownerClass, List children) {
		List childrenAll = new ArrayList(children);
		for (Object child: childrenAll) {
			if (child instanceof View) {
				View childView = (View) child;
				EObject childElement = childView.getElement();
				if (childElement.eContainer().eClass().isSuperTypeOf(ownerClass)) {
					children.remove(child);
				}
			}
		}
		return children;
	}
	
	/**
	 * Filter out children that don't have a particular object in their containment tree.
	 */
	@SuppressWarnings("unchecked")
	public static List filterChildrenWithObjectNotInContainmentTree(EObject container, List children) {
		List childrenAll = new ArrayList(children);
		for (Object child: childrenAll) {
			if (child instanceof View) {
				View childView = (View) child;
				EObject childElement = childView.getElement();
				if (! com.ociweb.emf.util.ObjectsFinder.hasObjectInContainmentTree(childElement, container)) {
					children.remove(child);
				}
			}
		}
		return children;
		
	}
}
