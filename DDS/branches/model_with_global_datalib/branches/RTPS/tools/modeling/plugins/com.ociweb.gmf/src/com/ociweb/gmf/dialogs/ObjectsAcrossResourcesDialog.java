package com.ociweb.gmf.dialogs;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.SortedSet;
import java.util.TreeSet;

import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.FontData;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;

/**
 * A custom JFace dialog to allow selection of EMF objects
 * that can reside in multiple resources. The dialog breaks
 * presents objects from the same resource together to
 * allow the user to easily confine the selection to
 * objects from one resource.
 */
public class ObjectsAcrossResourcesDialog <Obj extends EObject> extends Dialog
{
	private final String dialogTitle;
	
	private final Map<Obj, Boolean> objectSelection;

	private boolean singleItemSelectable = false;

	List<Button> checkboxes;

	ObjectLabeler<Obj> objectLabeler;

	/**
	 * A widget selection listener used to tie checkbox selection
	 * to the Object behind the checkbox.
	 */
	private class SelectionToggleListener extends SelectionAdapter {
		private final Obj objectForCheckbox;

		/**
		 * @param object The Object associated with the checkbox.
		 */
		public SelectionToggleListener(Obj object) {
			super();
			objectForCheckbox = object;
		}

		@Override
		public void widgetSelected(SelectionEvent e) {
			boolean checkboxSelected = ((Button) e.getSource()).getSelection();
			objectSelection.put(objectForCheckbox, checkboxSelected);
		}
	}

	public ObjectsAcrossResourcesDialog(
			String dialogTitle,
			Shell parentShell,
			Collection<Obj> objCandidates,
			ObjectLabeler<Obj> labeler)
	{
		super(parentShell);
		this.dialogTitle = dialogTitle;
		checkboxes = new ArrayList<Button>(objCandidates.size());
		objectSelection = new HashMap<Obj, Boolean>(objCandidates.size());
		for (Obj obj: objCandidates) {
			objectSelection.put(obj, false);
		}
		objectLabeler = labeler;
	}

	public void onlySingleItemSelectable() {
		this.singleItemSelectable = true;
	}


	/**
	 * Set the dialog title.
	 */
	@Override
	protected void configureShell(Shell shell) {
		super.configureShell(shell);
		shell.setText(dialogTitle);
	}

	public List<Obj> getObjectsSelected() {
		List<Obj> selection = new ArrayList<Obj>();
		for (Map.Entry<Obj, Boolean> entry: objectSelection.entrySet()) {
			if (entry.getValue()) {
				selection.add(entry.getKey());
			}
		}
		return selection;
	}

	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite comp = (Composite)super.createDialogArea(parent);

		// To present objects based on resource, populate a map to
		// go from a Resource to the Objects that are that resource.
		Map<Resource, TreeSet<Obj>> resourceToObjects = new HashMap<Resource, TreeSet<Obj>>();
		for (Map.Entry<Obj, Boolean> entry: objectSelection.entrySet()) {
			Obj object = entry.getKey();
			Resource resource = object.eResource();
			if (resourceToObjects.containsKey(resource)) {
				SortedSet<Obj> objects = resourceToObjects.get(resource);
				objects.add(object);
			}
			else {
				TreeSet<Obj> objects = new TreeSet<Obj>(objectLabeler);
				objects.add(object);
				resourceToObjects.put(resource, objects);
			}
		}

		// Put resources into a sorted set so that presentation order of resources
		// is controlled.
		ResourceComparator comparator = new ResourceComparator();
		SortedSet<Resource> resourceSet = new TreeSet<Resource>(comparator);
		for (Resource resource: resourceToObjects.keySet()) {
			resourceSet.add(resource);
		}

		for (Resource resource: resourceSet) {

			// Present the resource
			Label label = new Label(comp, SWT.RIGHT);
			label.setText(getResourceLabel(resource));
			FontData labelFontData =
				new FontData("Arial", 10, SWT.BOLD);
			label.setFont(new Font(parent.getDisplay(), labelFontData));

			// Present the objects in the resource
			for (Obj object: resourceToObjects.get(resource)) {
				int buttonType = singleItemSelectable ? SWT.RADIO : SWT.CHECK;
				Button checkbox = new Button(comp, buttonType);
				checkbox.setText(objectLabeler.getLabel(object));
				checkboxes.add(checkbox);
				checkbox.addSelectionListener(new SelectionToggleListener(object));
			}
		}
		return comp;
	}

	@Override
	protected void createButtonsForButtonBar(Composite parent)
	{
		super.createButtonsForButtonBar(parent);
		// "Reset All" doesn't fire a SelectionEvent for the
		// buttons so SelectionEvent being used will be bypassed.
		//		createButton(parent, IDialogConstants.NO_TO_ALL_ID + 1, "Reset All", false);
	}

	static String getResourceLabel(Resource resource) {
		return resource.getURI().path().replace("/resource/", "");
	}

}

class ResourceComparator implements Comparator<Resource> {

	public int compare(Resource res1, Resource res2) {
		String label1 = ObjectsAcrossResourcesDialog.getResourceLabel(res1);
		String label2 = ObjectsAcrossResourcesDialog.getResourceLabel(res2);

		return label1.compareTo(label2);
	}
}
