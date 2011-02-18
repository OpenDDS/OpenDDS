package org.opendds.modeling.sdk.model.GeneratorSpecification.Generator;

import javax.xml.transform.Source;

public interface IGeneratorModel {

	/**
	 * Set the EMF EditingDomain without needing to import any EMF classes.
	 * 
	 * @param editingDomain object containing the EMF model.
	 */
	public abstract void setEditingDomain( Object editingDomain);
	
	/**
	 * Set the contained ParsedModelFile.
	 * 
	 * @param parsedModelFile object containing the OpenDDS model.
	 */
	public abstract void setParsedModelFile( ParsedModelFile parsedModelFile);

	/**
	 * Determine if the element is the model source file in the model.
	 * 
	 * @param element candidate element to check.
	 * 
	 * @return boolean
	 */
	public abstract boolean isModelSource( Object element);
	
	/**
	 * Determine if the element is the model target dir in the model.
	 *
	 * @param element candidate element to check.
	 * 
	 * @return boolean
	 */
	public abstract boolean isModelTarget( Object element);

	/**
	 * Get the OpenDDS model file name.
	 * 
	 * @return String containing the name of the file containing the OpenDDS model to generate from.
	 */
	public abstract String getModelFileName();

	/**
	 * Set the OpenDDS model file name.
	 * 
	 * @param modelFileName the name of the file containing the OpenDDS model to generate from.
	 */
	public abstract void setModelFileName( String modelFileName);
	
	/**
	 * Get the generation target directory.
	 * 
	 * @return String containing the name of the directory where the output will be generated to.
	 */
	public abstract String getTargetDirName();
	
	/**
	 * Set the generation target directory.
	 * 
	 * @param targetDirName the name of the directory where output will be generated to.
	 */
	public abstract void setTargetDirName( String targetDirName);

	/**
	 * Return a the contained generator model in a form suitable for transformation.
	 * 
	 * @param transformer engine for resolving an OpenDDS model into a source suitable for further processing.
	 * 
	 * @return Source suitable for transformation.
	 */
	public abstract Source getSource( SdkTransformer transformer);

	/**
	 * Get the OpenDDS model name.
	 * 
	 * @return String containing the name of the model extracted from the model file.
	 */
	public abstract String getModelName();

	public abstract long getTimestamp();
}
