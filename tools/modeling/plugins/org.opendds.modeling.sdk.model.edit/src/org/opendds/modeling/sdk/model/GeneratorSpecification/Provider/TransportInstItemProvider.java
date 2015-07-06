/**
 * <copyright>
 * </copyright>
 *
 */
package org.opendds.modeling.sdk.model.GeneratorSpecification.Provider;

import java.util.Collection;
import java.util.List;

import org.eclipse.emf.common.notify.AdapterFactory;
import org.eclipse.emf.common.notify.Notification;

import org.eclipse.emf.common.util.ResourceLocator;

import org.eclipse.emf.ecore.EStructuralFeature;

import org.eclipse.emf.edit.provider.ComposeableAdapterFactory;
import org.eclipse.emf.edit.provider.IChildCreationExtender;
import org.eclipse.emf.edit.provider.IEditingDomainItemProvider;
import org.eclipse.emf.edit.provider.IItemLabelProvider;
import org.eclipse.emf.edit.provider.IItemPropertyDescriptor;
import org.eclipse.emf.edit.provider.IItemPropertySource;
import org.eclipse.emf.edit.provider.IStructuredItemContentProvider;
import org.eclipse.emf.edit.provider.ITreeItemContentProvider;
import org.eclipse.emf.edit.provider.ItemPropertyDescriptor;
import org.eclipse.emf.edit.provider.ItemProviderAdapter;
import org.eclipse.emf.edit.provider.ViewerNotification;

import org.opendds.modeling.common.Plugin;

import org.opendds.modeling.sdk.model.GeneratorSpecification.GeneratorFactory;
import org.opendds.modeling.sdk.model.GeneratorSpecification.GeneratorPackage;
import org.opendds.modeling.sdk.model.GeneratorSpecification.TransportInst;

/**
 * This is the item provider adapter for a {@link org.opendds.modeling.sdk.model.GeneratorSpecification.TransportInst} object.
 * <!-- begin-user-doc -->
 * <!-- end-user-doc -->
 * @generated
 */
public class TransportInstItemProvider extends ItemProviderAdapter implements
		IEditingDomainItemProvider, IStructuredItemContentProvider,
		ITreeItemContentProvider, IItemLabelProvider, IItemPropertySource {
	/**
	 * This constructs an instance from a factory and a notifier.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	public TransportInstItemProvider(AdapterFactory adapterFactory) {
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

			addNamePropertyDescriptor(object);
		}
		return itemPropertyDescriptors;
	}

	/**
	 * This adds a property descriptor for the Name feature.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	protected void addNamePropertyDescriptor(Object object) {
		itemPropertyDescriptors.add(createItemPropertyDescriptor(
				((ComposeableAdapterFactory) adapterFactory)
						.getRootAdapterFactory(),
				getResourceLocator(),
				getString("_UI_TransportInst_name_feature"),
				getString("_UI_PropertyDescriptor_description",
						"_UI_TransportInst_name_feature",
						"_UI_TransportInst_type"),
				GeneratorPackage.Literals.TRANSPORT_INST__NAME, true, false,
				false, ItemPropertyDescriptor.GENERIC_VALUE_IMAGE, null, null));
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
			childrenFeatures
					.add(GeneratorPackage.Literals.TRANSPORT_INST__QUEUE_MESSAGES_PER_POOL);
			childrenFeatures
					.add(GeneratorPackage.Literals.TRANSPORT_INST__QUEUE_INITIAL_POOLS);
			childrenFeatures
					.add(GeneratorPackage.Literals.TRANSPORT_INST__MAX_PACKET_SIZE);
			childrenFeatures
					.add(GeneratorPackage.Literals.TRANSPORT_INST__MAX_SAMPLES_PER_PACKET);
			childrenFeatures
					.add(GeneratorPackage.Literals.TRANSPORT_INST__OPTIMUM_PACKET_SIZE);
			childrenFeatures
					.add(GeneratorPackage.Literals.TRANSPORT_INST__THREAD_PER_CONNECTION);
			childrenFeatures
					.add(GeneratorPackage.Literals.TRANSPORT_INST__DATALINK_RELEASE_DELAY);
			childrenFeatures
					.add(GeneratorPackage.Literals.TRANSPORT_INST__DATALINK_CONTROL_CHUNKS);
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
	 * This returns the label text for the adapted class.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	@Override
	public String getText(Object object) {
		String label = ((TransportInst) object).getName();
		return label == null || label.length() == 0 ? getString("_UI_TransportInst_type")
				: getString("_UI_TransportInst_type") + " " + label;
	}

	/**
	 * This handles model notifications by calling {@link #updateChildren} to update any cached
	 * children and by creating a viewer notification, which it passes to {@link #fireNotifyChanged}.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated - NOT
	 */
	@Override
	public void notifyChanged(Notification notification) {
		notifyChangedGen(notification);
		switch (notification.getFeatureID(TransportInst.class)) {
		case GeneratorPackage.TRANSPORT_INST__NAME:
			fireNotifyChanged(new ViewerNotification(notification));
			return;
		}
	}

	/**
	 * This handles model notifications by calling {@link #updateChildren} to update any cached
	 * children and by creating a viewer notification, which it passes to {@link #fireNotifyChanged}.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	public void notifyChangedGen(Notification notification) {
		updateChildren(notification);

		switch (notification.getFeatureID(TransportInst.class)) {
		case GeneratorPackage.TRANSPORT_INST__NAME:
			fireNotifyChanged(new ViewerNotification(notification,
					notification.getNotifier(), false, true));
			return;
		case GeneratorPackage.TRANSPORT_INST__QUEUE_MESSAGES_PER_POOL:
		case GeneratorPackage.TRANSPORT_INST__QUEUE_INITIAL_POOLS:
		case GeneratorPackage.TRANSPORT_INST__MAX_PACKET_SIZE:
		case GeneratorPackage.TRANSPORT_INST__MAX_SAMPLES_PER_PACKET:
		case GeneratorPackage.TRANSPORT_INST__OPTIMUM_PACKET_SIZE:
		case GeneratorPackage.TRANSPORT_INST__THREAD_PER_CONNECTION:
		case GeneratorPackage.TRANSPORT_INST__DATALINK_RELEASE_DELAY:
		case GeneratorPackage.TRANSPORT_INST__DATALINK_CONTROL_CHUNKS:
			fireNotifyChanged(new ViewerNotification(notification,
					notification.getNotifier(), true, false));
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

		newChildDescriptors
				.add(createChildParameter(
						GeneratorPackage.Literals.TRANSPORT_INST__QUEUE_MESSAGES_PER_POOL,
						GeneratorFactory.eINSTANCE.createQueueMessagesPerPool()));

		newChildDescriptors.add(createChildParameter(
				GeneratorPackage.Literals.TRANSPORT_INST__QUEUE_INITIAL_POOLS,
				GeneratorFactory.eINSTANCE.createQueueInitialPool()));

		newChildDescriptors.add(createChildParameter(
				GeneratorPackage.Literals.TRANSPORT_INST__MAX_PACKET_SIZE,
				GeneratorFactory.eINSTANCE.createMaxPacketSize()));

		newChildDescriptors
				.add(createChildParameter(
						GeneratorPackage.Literals.TRANSPORT_INST__MAX_SAMPLES_PER_PACKET,
						GeneratorFactory.eINSTANCE.createMaxSamplesPerPacket()));

		newChildDescriptors.add(createChildParameter(
				GeneratorPackage.Literals.TRANSPORT_INST__OPTIMUM_PACKET_SIZE,
				GeneratorFactory.eINSTANCE.createOptimumPacketSize()));

		newChildDescriptors
				.add(createChildParameter(
						GeneratorPackage.Literals.TRANSPORT_INST__THREAD_PER_CONNECTION,
						GeneratorFactory.eINSTANCE.createThreadPerConnection()));

		newChildDescriptors
				.add(createChildParameter(
						GeneratorPackage.Literals.TRANSPORT_INST__DATALINK_RELEASE_DELAY,
						GeneratorFactory.eINSTANCE.createDatalinkReleaseDelay()));

		newChildDescriptors
				.add(createChildParameter(
						GeneratorPackage.Literals.TRANSPORT_INST__DATALINK_CONTROL_CHUNKS,
						GeneratorFactory.eINSTANCE
								.createDatalinkControlChunks()));
	}

	/**
	 * Return the resource locator for this item provider's resources.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	@Override
	public ResourceLocator getResourceLocator() {
		return ((IChildCreationExtender) adapterFactory).getResourceLocator();
	}

}
