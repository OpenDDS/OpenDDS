/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
package OpenDDS;

import java.util.Collection;
import java.util.List;

import org.eclipse.emf.common.notify.AdapterFactory;
import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.emf.edit.provider.IEditingDomainItemProvider;
import org.eclipse.emf.edit.provider.IItemLabelProvider;
import org.eclipse.emf.edit.provider.IItemPropertyDescriptor;
import org.eclipse.emf.edit.provider.IItemPropertySource;
import org.eclipse.emf.edit.provider.IStructuredItemContentProvider;
import org.eclipse.emf.edit.provider.ITreeItemContentProvider;
import org.eclipse.emf.edit.provider.ViewerNotification;

/**
 * This is the item provider adapter for a {@link OpenDDS.TopicStruct} object.
 * <!-- begin-user-doc -->
 * <!-- end-user-doc -->
 * @generated
 */
public class TopicStructItemProvider extends ConstructedTopicTypeItemProvider
		implements IEditingDomainItemProvider, IStructuredItemContentProvider,
		ITreeItemContentProvider, IItemLabelProvider, IItemPropertySource {
	/**
	 * This constructs an instance from a factory and a notifier.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	public TopicStructItemProvider(AdapterFactory adapterFactory) {
		super(adapterFactory);
	}

	/**
	 * This returns the property descriptors for the adapted class.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	@Override
	public List<IItemPropertyDescriptor> getPropertyDescriptors(Object object) {
		if (itemPropertyDescriptors == null) {
			super.getPropertyDescriptors(object);

		}
		return itemPropertyDescriptors;
	}

	/**
	 * This specifies how to implement {@link #getChildren} and is used to deduce an appropriate feature for an
	 * {@link org.eclipse.emf.edit.command.AddCommand}, {@link org.eclipse.emf.edit.command.RemoveCommand} or
	 * {@link org.eclipse.emf.edit.command.MoveCommand} in {@link #createCommand}.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	@Override
	public Collection<? extends EStructuralFeature> getChildrenFeatures(
			Object object) {
		if (childrenFeatures == null) {
			super.getChildrenFeatures(object);
			childrenFeatures.add(ModelPackage.Literals.TOPIC_STRUCT__MEMBERS);
			childrenFeatures.add(ModelPackage.Literals.TOPIC_STRUCT__KEYS);
		}
		return childrenFeatures;
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	@Override
	protected EStructuralFeature getChildFeature(Object object, Object child) {
		// Check the type of the specified child object and return the proper feature to use for
		// adding (see {@link AddCommand}) it as a child.

		return super.getChildFeature(object, child);
	}

	/**
	 * This returns TopicStruct.gif.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	@Override
	public Object getImage(Object object) {
		return overlayImage(object, getResourceLocator().getImage(
				"full/obj16/TopicStruct"));
	}

	/**
	 * This returns the label text for the adapted class.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	@Override
	public String getText(Object object) {
		String label = ((TopicStruct) object).getName();
		return label == null || label.length() == 0 ? getString("_UI_TopicStruct_type")
				: getString("_UI_TopicStruct_type") + " " + label;
	}

	/**
	 * This handles model notifications by calling {@link #updateChildren} to update any cached
	 * children and by creating a viewer notification, which it passes to {@link #fireNotifyChanged}.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	@Override
	public void notifyChanged(Notification notification) {
		updateChildren(notification);

		switch (notification.getFeatureID(TopicStruct.class)) {
		case ModelPackage.TOPIC_STRUCT__MEMBERS:
		case ModelPackage.TOPIC_STRUCT__KEYS:
			fireNotifyChanged(new ViewerNotification(notification, notification
					.getNotifier(), true, false));
			return;
		}
		super.notifyChanged(notification);
	}

	/**
	 * This adds {@link org.eclipse.emf.edit.command.CommandParameter}s describing the children
	 * that can be created under this object.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	@Override
	protected void collectNewChildDescriptors(
			Collection<Object> newChildDescriptors, Object object) {
		super.collectNewChildDescriptors(newChildDescriptors, object);

		newChildDescriptors.add(createChildParameter(
				ModelPackage.Literals.TOPIC_STRUCT__MEMBERS,
				ModelFactory.eINSTANCE.createArray()));

		newChildDescriptors.add(createChildParameter(
				ModelPackage.Literals.TOPIC_STRUCT__MEMBERS,
				ModelFactory.eINSTANCE.createOBoolean()));

		newChildDescriptors.add(createChildParameter(
				ModelPackage.Literals.TOPIC_STRUCT__MEMBERS,
				ModelFactory.eINSTANCE.createCase()));

		newChildDescriptors.add(createChildParameter(
				ModelPackage.Literals.TOPIC_STRUCT__MEMBERS,
				ModelFactory.eINSTANCE.createOChar()));

		newChildDescriptors.add(createChildParameter(
				ModelPackage.Literals.TOPIC_STRUCT__MEMBERS,
				ModelFactory.eINSTANCE.createODouble()));

		newChildDescriptors.add(createChildParameter(
				ModelPackage.Literals.TOPIC_STRUCT__MEMBERS,
				ModelFactory.eINSTANCE.createEnum()));

		newChildDescriptors.add(createChildParameter(
				ModelPackage.Literals.TOPIC_STRUCT__MEMBERS,
				ModelFactory.eINSTANCE.createOFloat()));

		newChildDescriptors.add(createChildParameter(
				ModelPackage.Literals.TOPIC_STRUCT__MEMBERS,
				ModelFactory.eINSTANCE.createOLong()));

		newChildDescriptors.add(createChildParameter(
				ModelPackage.Literals.TOPIC_STRUCT__MEMBERS,
				ModelFactory.eINSTANCE.createOLongLong()));

		newChildDescriptors.add(createChildParameter(
				ModelPackage.Literals.TOPIC_STRUCT__MEMBERS,
				ModelFactory.eINSTANCE.createOOctet()));

		newChildDescriptors.add(createChildParameter(
				ModelPackage.Literals.TOPIC_STRUCT__MEMBERS,
				ModelFactory.eINSTANCE.createSequence()));

		newChildDescriptors.add(createChildParameter(
				ModelPackage.Literals.TOPIC_STRUCT__MEMBERS,
				ModelFactory.eINSTANCE.createOShort()));

		newChildDescriptors.add(createChildParameter(
				ModelPackage.Literals.TOPIC_STRUCT__MEMBERS,
				ModelFactory.eINSTANCE.createOString()));

		newChildDescriptors.add(createChildParameter(
				ModelPackage.Literals.TOPIC_STRUCT__MEMBERS,
				ModelFactory.eINSTANCE.createTopicStruct()));

		newChildDescriptors.add(createChildParameter(
				ModelPackage.Literals.TOPIC_STRUCT__MEMBERS,
				ModelFactory.eINSTANCE.createTypedef()));

		newChildDescriptors.add(createChildParameter(
				ModelPackage.Literals.TOPIC_STRUCT__MEMBERS,
				ModelFactory.eINSTANCE.createUnion()));

		newChildDescriptors.add(createChildParameter(
				ModelPackage.Literals.TOPIC_STRUCT__KEYS,
				ModelFactory.eINSTANCE.createKey()));
	}

}
