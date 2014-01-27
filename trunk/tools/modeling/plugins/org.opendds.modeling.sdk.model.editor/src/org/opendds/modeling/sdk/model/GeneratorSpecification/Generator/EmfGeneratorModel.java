package org.opendds.modeling.sdk.model.GeneratorSpecification.Generator;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.HashMap;
import java.util.Map;

import javax.xml.transform.Source;
import javax.xml.transform.stream.StreamSource;

import org.eclipse.emf.common.command.BasicCommandStack;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.emf.ecore.xmi.XMLResource;
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

	private Resource getCodegenResource() {
		EList<Resource> resources = editingDomain.getResourceSet().getResources();
		if (resources.size() > 0) {
			return resources.get(0);
		}
		return null;
	}

	@Override
	public void setEditingDomain(Object editingDomain) {
		if( editingDomain != null && editingDomain instanceof EditingDomain) {
			this.editingDomain = (EditingDomain)editingDomain;

			// Extract the model file and target dir model elements.
			//
			Resource resource = getCodegenResource();
			if (resource != null) {
				EList<EObject> contents = resource.getContents();
				if( contents.size() > 0) {
					EObject root = contents.get(0);
					if( root instanceof CodeGen) {
						modelSource = ((CodeGen)root).getSource();
						if( parsedModelFile != null) {
							parsedModelFile.setSourceName(modelSource.getName());
						}
						
						modelTarget = ((CodeGen)root).getTarget();
					}
				}
			}
		}
	}
	
	@Override
	public void setParsedModelFile( ParsedModelFile parsedModelFile) {
		if( parsedModelFile != null) {
			this.parsedModelFile = parsedModelFile;
			if( modelSource != null) {
				parsedModelFile.setSourceName(modelSource.getName());
				
			}
		}
	}

	@Override
	public void setModelFileName(String modelFileName) {
		String oldName = parsedModelFile.getSourceName();
		if( (modelFileName != null && oldName != null
		     && modelFileName.equals(parsedModelFile.getSourceName()))
		 || (modelFileName == null && oldName == null)) {
			// Don't update if no change.
			return;
		}

		if( parsedModelFile != null) {
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
		String oldName = modelTarget.getName();
		if( (targetDirName != null && modelTarget.getName() != null
 	         && targetDirName.equals(modelTarget.getName()))
 	     || (targetDirName == null && oldName == null)) {
			// Don't update if no change.
			return;
		}
		
		if( editingDomain != null && modelTarget != null) {
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

		Resource resource = getCodegenResource();
		if (resource == null) {
			return null;
		}

		ByteArrayOutputStream baos = new ByteArrayOutputStream();
		final Map<Object, Object> saveOptions = new HashMap<Object, Object>();
		saveOptions.put(Resource.OPTION_SAVE_ONLY_IF_CHANGED,
				Resource.OPTION_SAVE_ONLY_IF_CHANGED_MEMORY_BUFFER);
		saveOptions.put(XMLResource.OPTION_KEEP_DEFAULT_CONTENT, Boolean.TRUE);
		try {
			resource.save(baos, saveOptions);
		} catch (IOException e) {
			// No actual I/O is happening because it's a ByteArrayOutputStream
		}
		return new StreamSource(new ByteArrayInputStream(baos.toByteArray()));
	}

	@Override
	public long getTimestamp() {
		if (((BasicCommandStack) editingDomain.getCommandStack()).isSaveNeeded()) {
			return 0; // understood by SdkGenerator to indicate an unknown timestamp
		}
		Resource resource = getCodegenResource();
		return resource == null ? 0 : resource.getTimeStamp();
	}

}
