package org.opendds.modeling.sdk.codegen;

import org.eclipse.jface.action.*;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IEditorPart;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.actions.ActionFactory;
import org.eclipse.ui.forms.editor.FormEditor;
import org.eclipse.ui.ide.IDE;
import org.eclipse.ui.ide.IDEActionFactory;
import org.eclipse.ui.part.MultiPageEditorActionBarContributor;
import org.eclipse.ui.part.MultiPageEditorPart;
import org.eclipse.ui.texteditor.ITextEditor;
import org.eclipse.ui.texteditor.ITextEditorActionConstants;

/**
 * Manages the installation/deinstallation of global actions for multi-page editors.
 * Responsible for the redirection of global actions to the active editor.
 * Multi-page contributor replaces the contributors for the individual editors in the multi-page editor.
 */
public class CodeGenEditorContributor extends MultiPageEditorActionBarContributor {
	private IEditorPart activeEditorPart;
	private CodeGenEditor editor;
	private Action sampleAction;
	
	private Action generateIdl;
	private Action generateH;
	private Action generateCpp;
	private Action generateMpc;
	
	/**
	 * Creates a multi-page contributor.
	 */
	public CodeGenEditorContributor() {
		super();
		createActions();
	}
	/**
	 * Returns the action registed with the given text editor.
	 * @return IAction or null if editor is null.
	 */
	protected IAction getAction(ITextEditor editor, String actionID) {
		return (editor == null ? null : editor.getAction(actionID));
	}
	
	public void setActiveEditor(IEditorPart part) {
		
		if( part == null) {
			System.out.println("The Editor Part is NULL!");
		} else if( part instanceof CodeGenEditor) {
			System.out.println("Its A CodeGenEditor!");
		} else if( part instanceof FormEditor) {
			System.out.println("Its A FormEditor!");
		} else if( part instanceof MultiPageEditorPart) {
			System.out.println("Its A MultiPageEditorPart!");
		} else if( part instanceof OutputsForm) {
			System.out.println("Its An OutputsForm!");
		} else {
			System.out.println("Its A " + part.toString() + "!");
		}

		super.setActiveEditor(part);
	}
	/* (non-JavaDoc)
	 * Method declared in AbstractMultiPageEditorActionBarContributor.
	 */

	public void setActivePage(IEditorPart part) {
		if (activeEditorPart == part)
			return;

		activeEditorPart = part;
		
		if( part == null) {
			System.out.println("The Editor Part is NULL!");
		} else if( part instanceof CodeGenEditor) {
			System.out.println("Its A CodeGenEditor!");
		} else if( part instanceof FormEditor) {
			System.out.println("Its A FormEditor!");
		} else if( part instanceof MultiPageEditorPart) {
			System.out.println("Its A MultiPageEditorPart!");
		} else if( part instanceof OutputsForm) {
			System.out.println("Its An OutputsForm!");
		} else {
			System.out.println("Its A " + part.toString() + "!");
		}

		IActionBars actionBars = getActionBars();
		if (actionBars != null) {

			ITextEditor editor = (part instanceof ITextEditor) ? (ITextEditor) part : null;

			actionBars.setGlobalActionHandler(
				ActionFactory.DELETE.getId(),
				getAction(editor, ITextEditorActionConstants.DELETE));
			actionBars.setGlobalActionHandler(
				ActionFactory.UNDO.getId(),
				getAction(editor, ITextEditorActionConstants.UNDO));
			actionBars.setGlobalActionHandler(
				ActionFactory.REDO.getId(),
				getAction(editor, ITextEditorActionConstants.REDO));
			actionBars.setGlobalActionHandler(
				ActionFactory.CUT.getId(),
				getAction(editor, ITextEditorActionConstants.CUT));
			actionBars.setGlobalActionHandler(
				ActionFactory.COPY.getId(),
				getAction(editor, ITextEditorActionConstants.COPY));
			actionBars.setGlobalActionHandler(
				ActionFactory.PASTE.getId(),
				getAction(editor, ITextEditorActionConstants.PASTE));
			actionBars.setGlobalActionHandler(
				ActionFactory.SELECT_ALL.getId(),
				getAction(editor, ITextEditorActionConstants.SELECT_ALL));
			actionBars.setGlobalActionHandler(
				ActionFactory.FIND.getId(),
				getAction(editor, ITextEditorActionConstants.FIND));
			actionBars.setGlobalActionHandler(
				IDEActionFactory.BOOKMARK.getId(),
				getAction(editor, IDEActionFactory.BOOKMARK.getId()));
			actionBars.updateActionBars();
		}
	}
	private void createActions() {
		sampleAction = new Action() {
			public void run() {
				MessageDialog.openInformation(null, "OpenDDS Modeling SDK", "Generating Something!");
			}
		};
		sampleAction.setText("OpenDDS");
		sampleAction.setToolTipText("Generate code from a model");
		sampleAction.setImageDescriptor(PlatformUI.getWorkbench().getSharedImages().
				getImageDescriptor(IDE.SharedImages.IMG_OBJS_TASK_TSK));
		System.out.println("Action contemplated!");
	}
	public void contributeToMenu(IMenuManager manager) {
		IMenuManager menu = new MenuManager("Editor &Menu");
		manager.prependToGroup(IWorkbenchActionConstants.MB_ADDITIONS, menu);
		menu.add(sampleAction);
		System.out.println("Contribution made!");
	}
	public void contributeToToolBar(IToolBarManager manager) {
		manager.add(new Separator());
		manager.add(sampleAction);
	}
}
