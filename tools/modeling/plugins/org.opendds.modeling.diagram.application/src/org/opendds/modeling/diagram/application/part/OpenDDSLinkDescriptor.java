package org.opendds.modeling.diagram.application.part;

import org.eclipse.core.runtime.IAdaptable;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.gmf.runtime.emf.core.util.EObjectAdapter;
import org.eclipse.gmf.runtime.emf.type.core.IElementType;

/**
 * @generated
 */
public class OpenDDSLinkDescriptor extends OpenDDSNodeDescriptor {

	/**
	 * @generated
	 */
	private EObject mySource;

	/**
	 * @generated
	 */
	private EObject myDestination;

	/**
	 * @generated
	 */
	private IAdaptable mySemanticAdapter;

	/**
	 * @generated
	 */
	private OpenDDSLinkDescriptor(EObject source, EObject destination,
			EObject linkElement, int linkVID) {
		super(linkElement, linkVID);
		mySource = source;
		myDestination = destination;
	}

	/**
	 * @generated
	 */
	public OpenDDSLinkDescriptor(EObject source, EObject destination,
			IElementType elementType, int linkVID) {
		this(source, destination, (EObject) null, linkVID);
		final IElementType elementTypeCopy = elementType;
		mySemanticAdapter = new IAdaptable() {
			public Object getAdapter(Class adapter) {
				if (IElementType.class.equals(adapter)) {
					return elementTypeCopy;
				}
				return null;
			}
		};
	}

	/**
	 * @generated
	 */
	public OpenDDSLinkDescriptor(EObject source, EObject destination,
			EObject linkElement, IElementType elementType, int linkVID) {
		this(source, destination, linkElement, linkVID);
		final IElementType elementTypeCopy = elementType;
		mySemanticAdapter = new EObjectAdapter(linkElement) {
			@Override
			public Object getAdapter(Class adapter) {
				if (IElementType.class.equals(adapter)) {
					return elementTypeCopy;
				}
				return super.getAdapter(adapter);
			}
		};
	}

	/**
	 * @generated
	 */
	public EObject getSource() {
		return mySource;
	}

	/**
	 * @generated
	 */
	public EObject getDestination() {
		return myDestination;
	}

	/**
	 * @generated
	 */
	public IAdaptable getSemanticAdapter() {
		return mySemanticAdapter;
	}

}
