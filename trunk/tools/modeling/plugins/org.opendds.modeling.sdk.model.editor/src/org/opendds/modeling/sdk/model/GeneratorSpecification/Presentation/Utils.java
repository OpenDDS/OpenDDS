package org.opendds.modeling.sdk.model.GeneratorSpecification.Presentation;

import org.eclipse.core.resources.IFile;
import org.eclipse.core.resources.ResourcesPlugin;
import org.eclipse.core.runtime.IPath;
import org.eclipse.jface.viewers.ITreeSelection;
import org.eclipse.jface.viewers.TreePath;
import org.eclipse.jface.viewers.TreeSelection;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.dialogs.ContainerSelectionDialog;
import org.eclipse.ui.dialogs.ElementTreeSelectionDialog;
import org.eclipse.ui.model.BaseWorkbenchContentProvider;
import org.eclipse.ui.model.WorkbenchLabelProvider;

public class Utils {

	static public IPath browseForTargetDir( Composite parent, Object[] initialSelection) {
		ContainerSelectionDialog dialog = new ContainerSelectionDialog(
				parent.getShell(), ResourcesPlugin.getWorkspace().getRoot(), false,
				"Select target folder:");
		dialog.setTitle("Target Selection");
		if( initialSelection != null) {
			dialog.setInitialSelections( initialSelection);
		}
		if (dialog.open() == ContainerSelectionDialog.OK) {
			Object[] result = dialog.getResult();
			if (result.length == 1) {
				return (IPath)result[0];
			}
		}
		return null;
	}

	static public IPath browseForModelFile( Composite parent, IPath current) {
		ElementTreeSelectionDialog dialog = new ElementTreeSelectionDialog(
				parent.getShell(),
				new WorkbenchLabelProvider(),
				new BaseWorkbenchContentProvider());
		dialog.setTitle("Model Selection");
		dialog.setMessage("Select Model file:");
		dialog.setInput(ResourcesPlugin.getWorkspace().getRoot());
		if( current != null) {
			IPath path = current;
			if( path.segmentCount() > 1) {
				path = current.removeLastSegments(1);
				TreePath treePath = new TreePath( path.segments());
				ITreeSelection treeSelection = new TreeSelection( treePath);
				dialog.setInitialSelection( treeSelection);
			}
		}
		if(dialog.open() == ElementTreeSelectionDialog.OK) {
			Object[] result = dialog.getResult();
			if( result.length == 1) {
				Object r0 = result[0];
				if( r0 instanceof IFile) {
					return ((IFile)r0).getFullPath();
//					IPath modelPath = ((IFile)r0).getFullPath();
//					final IPath basePath = getGenFile().getFullPath();
//					URI modelURI = URI.createPlatformResourceURI(modelPath.toOSString(), false);
//					URI baseURI  = URI.createPlatformResourceURI(basePath.toString(),false);
//					URI relativeURI = modelURI.deresolve(baseURI);
//					return Path.fromOSString(relativeURI.toFileString());
				}
			}
		}
		return null;
	}

}
