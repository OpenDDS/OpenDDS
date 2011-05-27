package com.ociweb.gmf.part;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.NullProgressMonitor;
import org.eclipse.core.runtime.Status;
import org.eclipse.ui.IPartListener2;
import org.eclipse.ui.IWorkbenchPartReference;
import org.eclipse.ui.part.EditorPart;
import org.eclipse.ui.plugin.AbstractUIPlugin;

/**
 * Saves domain model file and/or graphics file if dirty
 * when an EditPart is deactivated.
 * This can be used as a workaround if diagram-subdiagram
 * synchronization is not in place. The idea being that it
 * is better to aggressively save changes that the user
 * may not be ready to finalize than to not be able to 
 * save changes in one editor because of conflicts with
 * another editor using the same resource(s). 
 */
public class SaveOnDeactivationListener implements IPartListener2 {

	private final EditorPart part;
	private final String partId;
	private final AbstractUIPlugin plugin;

	public SaveOnDeactivationListener(EditorPart part, String partId, AbstractUIPlugin plugin) {
		this.part = part;
		this.partId = partId;
		this.plugin = plugin;
	}

	private void saveIfDirty(IWorkbenchPartReference partRef) {
		if (partRef.getPart(false) == part) {
			if (part.isDirty()) {
				String message = "Editor deactivated- saving";
				plugin.getLog().log(
						new Status(IStatus.INFO, partId,
								IStatus.OK, message, null));
				IProgressMonitor monitor = new NullProgressMonitor();
				part.doSave(monitor);
			}
		}
	}

	@Override
	public void partActivated(IWorkbenchPartReference partRef) {
		// Ignore
	}

	@Override
	public void partBroughtToTop(IWorkbenchPartReference partRef) {
		// Ignore
	}

	@Override
	public void partClosed(IWorkbenchPartReference partRef) {
		// Ignore
	}

	@Override
	public void partDeactivated(IWorkbenchPartReference partRef) {
		saveIfDirty(partRef);
	}

	@Override
	public void partHidden(IWorkbenchPartReference partRef) {
		// Ignore
		// Part is deactivated as it becomes hidden.
	}

	@Override
	public void partInputChanged(IWorkbenchPartReference partRef) {
		// Ignore
	}

	@Override
	public void partOpened(IWorkbenchPartReference partRef) {
		// Ignore
	}

	@Override
	public void partVisible(IWorkbenchPartReference partRef) {
		// Ignore
	}

}
