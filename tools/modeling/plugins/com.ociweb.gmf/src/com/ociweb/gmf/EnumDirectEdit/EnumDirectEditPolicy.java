/*
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

/*
 * TODO: Submit this code to the Eclipe Foundation to use as part
 * of the resolution to bug "Support enum based labels" (https://bugs.eclipse.org/bugs/show_bug.cgi?id=158116).
 */

package com.ociweb.gmf.EnumDirectEdit;

import org.eclipse.emf.ecore.EObject;
import org.eclipse.gef.commands.Command;
import org.eclipse.gef.editpolicies.DirectEditPolicy;
import org.eclipse.gef.requests.DirectEditRequest;

import org.eclipse.gmf.runtime.common.core.command.ICommand;
import org.eclipse.gmf.runtime.diagram.ui.commands.ICommandProxy;
import org.eclipse.gmf.runtime.diagram.ui.editparts.ITextAwareEditPart;
import org.eclipse.gmf.runtime.diagram.core.util.ViewUtil;
import org.eclipse.gmf.runtime.emf.core.util.EObjectAdapter;
import org.eclipse.gmf.runtime.notation.View;

/**
 * A Direct Edit Policy to allow the selection in a GMF figure for a EMF Enum Literal.
 * @see EnumDirectEditManager
 * @author melaasar (wrote LabeDirectEditPolicy that this is adapted from)
 * @author harrisb@ociweb.com (adapted to handle Enums)
 */
public class EnumDirectEditPolicy
	extends DirectEditPolicy {

    private String[] enumNames;

	/**
	 * We need an adapter that will be able to hold both a model 
	 * and an view
	 */
	class EObjectAdapterEx
		extends EObjectAdapter {

		private View view = null;

		/**
		 * constructor
		 * @param element	element to be wrapped
		 * @param view	view to be wrapped
		 */
		public EObjectAdapterEx(EObject element, View view) {
			super(element);
			this.view = view;
		}

		@SuppressWarnings("unchecked")
		@Override
		public Object getAdapter(Class adapter) {
			Object o = super.getAdapter(adapter);
			if (o != null)
				return o;
			if (adapter.equals(View.class)) {
				return view;
			}
			return null;
		}
	}

	public EnumDirectEditPolicy(Class<? extends Enum<?>> e)
	{
		enumNames = EnumDirectEditManager.getEnumNames(e);
	}

	/**
	 * @see DirectEditPolicy#getDirectEditCommand(DirectEditRequest)
	 */
	@Override
	protected Command getDirectEditCommand(DirectEditRequest edit) {
		int index = (Integer) edit.getCellEditor().getValue();
		// If the blank item is selected -1 is returned. Pick first item in
		// this case.
		index = Math.max(0, index);
		String labelText = enumNames[index];
		
		//for CellEditor, null is always returned for invalid values
		if (labelText == null) {
			return null;
		}
		
		ITextAwareEditPart compartment = (ITextAwareEditPart) getHost();
		EObject model = (EObject)compartment.getModel();
		EObjectAdapter elementAdapter = null ;
		if (model instanceof View) {
            View view = (View)model;
			elementAdapter = new EObjectAdapterEx(ViewUtil.resolveSemanticElement(view),
				view);
        }
		else
			elementAdapter = new EObjectAdapterEx(model, null);
		// check to make sure an edit has occurred before returning a command.
		String prevText = compartment.getParser().getEditString(elementAdapter,
			compartment.getParserOptions().intValue());
		if (!prevText.equals(labelText)) {
			ICommand iCommand = 
				compartment.getParser().getParseCommand(elementAdapter, labelText, 0);
			return new ICommandProxy(iCommand);
		}

		return null;
	}

	/**
	 * @see DirectEditPolicy#showCurrentEditValue(DirectEditRequest)
	 */
	@Override
	protected void showCurrentEditValue(DirectEditRequest request) {
		int index = (Integer) request.getCellEditor().getValue();
		String value = enumNames[index];
		((ITextAwareEditPart) getHost()).setLabelText(value);
	}

}
