package org.opendds.modeling.sdk.model.GeneratorSpecification.Generator;

import javax.xml.transform.Source;

import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.emf.edit.command.SetCommand;
import org.eclipse.emf.edit.domain.EditingDomain;
import org.opendds.modeling.sdk.model.GeneratorSpecification.CodeGen;
import org.opendds.modeling.sdk.model.GeneratorSpecification.GeneratorPackage;
import org.opendds.modeling.sdk.model.GeneratorSpecification.ModelFile;
import org.opendds.modeling.sdk.model.GeneratorSpecification.TargetDir;

public class EmfGeneratorModel implements IGeneratorModel {

	/**
	 * The EMF model element for the OpenDDS editor model model file.
	 */
	protected ModelFile modelSource;
	
	/**
	 * The EMF model element for the OpenDDS editor model target directory.
	 */
	protected TargetDir modelTarget;

	/**
	 *  The editing domain containing the full EMF model we are working with.
	 */
	protected EditingDomain editingDomain = null;
	
	/**
	 *  This holds the reference to the ParsedModelFile since it 'owns' the model.
	 */
	protected ParsedModelFile parsedModelFile = null;

	@Override
	public String getModelFileName() {
		if( parsedModelFile != null) {
			return parsedModelFile.getSourceName();
		}
		return null;
	}

	@Override
	public String getModelName() {
		if( parsedModelFile != null) {
			return parsedModelFile.getModelName();
		}
		return null;
	}

	@Override
	public String getTargetDirName() {
		if( modelTarget != null) {
			return modelTarget.getName();
		}
		return null;
	}

	@Override
	public boolean isModelSource(Object element) {
		return element == modelSource;
	}

	@Override
	public boolean isModelTarget(Object element) {
		return element == modelTarget;
	}

	@Override
	public void setEditingDomain(Object editingDomain) {
		if( editingDomain != null && editingDomain instanceof EditingDomain) {
			this.editingDomain = (EditingDomain)editingDomain;

			// Extract the model file and target dir model elements.
			//
			EList<Resource> resources = this.editingDomain.getResourceSet().getResources();
			if( resources.size() > 0) {
				Resource resource = resources.get(0);
				EList<EObject> contents = resource.getContents();
				if( contents.size() > 0) {
					EObject root = contents.get(0);
					if( root instanceof CodeGen) {
						modelSource = ((CodeGen)root).getSource();
						setModelFileName( modelSource.getName());
						
						modelTarget = ((CodeGen)root).getTarget();
						setTargetDirName(modelTarget.getName());
					}
				}
			}
		}
	}
	
	@Override
	public void setParsedModelFile( ParsedModelFile parsedModelFile) {
		if( parsedModelFile != null) {
			this.parsedModelFile = parsedModelFile;
		}
	}

	@Override
	public void setModelFileName(String modelFileName) {
		if( parsedModelFile != null) {
			String oldName = parsedModelFile.getSourceName();
			if( (modelFileName != null && oldName != null
			     && modelFileName.equals(parsedModelFile.getSourceName()))
			 || (modelFileName == null && oldName == null)) {
				// Don't update if no change.
				return;
			}
			
			parsedModelFile.setSourceName(modelFileName);
		}
		
		if( editingDomain != null && modelSource != null) {
			editingDomain.getCommandStack().execute(
					SetCommand.create(
							editingDomain,
							modelSource,
							GeneratorPackage.eINSTANCE.getModelFile_Name(),
							modelFileName
					));
		}
	}

	@Override
	public void setTargetDirName(String targetDirName) {
		if( editingDomain != null && modelTarget != null) {
			String oldName = modelTarget.getName();
			if( (targetDirName != null && modelTarget.getName() != null
	 	         && targetDirName.equals(modelTarget.getName()))
	 	     || (targetDirName == null && oldName == null)) {
				// Don't update if no change.
				return;
			}
			
			editingDomain.getCommandStack().execute(
					SetCommand.create(
							editingDomain,
							modelTarget,
							GeneratorPackage.eINSTANCE.getTargetDir_Name(),
							targetDirName
					));
		}
	}
	
	@Override
	public Source getSource( SdkTransformer transformer) {
		if( transformer != null) {
			return parsedModelFile.getSource( transformer);
		}

		return null;

		// TODO Implement this.
//		if( source == null) {
//			Document doc = getModelDocument();
//			if (doc == null) {
//				return null; // messages were generated in the get call.
//			}
//
//			source = new DOMSource( doc);
//		}
//
//		return source;
	}

}
