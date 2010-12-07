package com.ociweb.gmf.dialogs;

import java.util.Comparator;

import org.eclipse.emf.ecore.EObject;

/**
 * Given an EMF EObject, return a String that can be used
 * as a label for the EObject. Also support comparison so
 * labels can be presented in order.
 *
 */
abstract public class ObjectLabeler <Obj extends EObject> implements Comparator<Obj>{

	public abstract String getLabel(Obj obj);

	public int compare(Obj obj1, Obj obj2) {
		String label1 = getLabel(obj1);
		String label2 = getLabel(obj2);
		return label1.compareTo(label2);
	}

}
