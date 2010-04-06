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
import org.eclipse.emf.edit.provider.ComposeableAdapterFactory;
import org.eclipse.emf.edit.provider.IEditingDomainItemProvider;
import org.eclipse.emf.edit.provider.IItemLabelProvider;
import org.eclipse.emf.edit.provider.IItemPropertyDescriptor;
import org.eclipse.emf.edit.provider.IItemPropertySource;
import org.eclipse.emf.edit.provider.IStructuredItemContentProvider;
import org.eclipse.emf.edit.provider.ITreeItemContentProvider;
import org.eclipse.emf.edit.provider.ItemPropertyDescriptor;
import org.eclipse.emf.edit.provider.ViewerNotification;

/**
 * This is the item provider adapter for a {@link OpenDDS.DurabilityServiceQosPolicy} object.
 * <!-- begin-user-doc -->
 * <!-- end-user-doc -->
 * @generated
 */
public class DurabilityServiceQosPolicyItemProvider extends
		QosPolicyItemProvider implements IEditingDomainItemProvider,
		IStructuredItemContentProvider, ITreeItemContentProvider,
		IItemLabelProvider, IItemPropertySource {
	/**
	 * This constructs an instance from a factory and a notifier.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	public DurabilityServiceQosPolicyItemProvider(AdapterFactory adapterFactory) {
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

			addHistory_depthPropertyDescriptor(object);
			addHistory_kindPropertyDescriptor(object);
			addMax_instancesPropertyDescriptor(object);
			addMax_samplesPropertyDescriptor(object);
			addMax_samples_per_instancePropertyDescriptor(object);
		}
		return itemPropertyDescriptors;
	}

	/**
	 * This adds a property descriptor for the History depth feature.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	protected void addHistory_depthPropertyDescriptor(Object object) {
		itemPropertyDescriptors
				.add(createItemPropertyDescriptor(
						((ComposeableAdapterFactory) adapterFactory)
								.getRootAdapterFactory(),
						getResourceLocator(),
						getString("_UI_DurabilityServiceQosPolicy_history_depth_feature"),
						getString(
								"_UI_PropertyDescriptor_description",
								"_UI_DurabilityServiceQosPolicy_history_depth_feature",
								"_UI_DurabilityServiceQosPolicy_type"),
						ModelPackage.Literals.DURABILITY_SERVICE_QOS_POLICY__HISTORY_DEPTH,
						true, false, false,
						ItemPropertyDescriptor.INTEGRAL_VALUE_IMAGE, null, null));
	}

	/**
	 * This adds a property descriptor for the History kind feature.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	protected void addHistory_kindPropertyDescriptor(Object object) {
		itemPropertyDescriptors
				.add(createItemPropertyDescriptor(
						((ComposeableAdapterFactory) adapterFactory)
								.getRootAdapterFactory(),
						getResourceLocator(),
						getString("_UI_DurabilityServiceQosPolicy_history_kind_feature"),
						getString(
								"_UI_PropertyDescriptor_description",
								"_UI_DurabilityServiceQosPolicy_history_kind_feature",
								"_UI_DurabilityServiceQosPolicy_type"),
						ModelPackage.Literals.DURABILITY_SERVICE_QOS_POLICY__HISTORY_KIND,
						true, false, false,
						ItemPropertyDescriptor.GENERIC_VALUE_IMAGE, null, null));
	}

	/**
	 * This adds a property descriptor for the Max instances feature.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	protected void addMax_instancesPropertyDescriptor(Object object) {
		itemPropertyDescriptors
				.add(createItemPropertyDescriptor(
						((ComposeableAdapterFactory) adapterFactory)
								.getRootAdapterFactory(),
						getResourceLocator(),
						getString("_UI_DurabilityServiceQosPolicy_max_instances_feature"),
						getString(
								"_UI_PropertyDescriptor_description",
								"_UI_DurabilityServiceQosPolicy_max_instances_feature",
								"_UI_DurabilityServiceQosPolicy_type"),
						ModelPackage.Literals.DURABILITY_SERVICE_QOS_POLICY__MAX_INSTANCES,
						true, false, false,
						ItemPropertyDescriptor.INTEGRAL_VALUE_IMAGE, null, null));
	}

	/**
	 * This adds a property descriptor for the Max samples feature.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	protected void addMax_samplesPropertyDescriptor(Object object) {
		itemPropertyDescriptors
				.add(createItemPropertyDescriptor(
						((ComposeableAdapterFactory) adapterFactory)
								.getRootAdapterFactory(),
						getResourceLocator(),
						getString("_UI_DurabilityServiceQosPolicy_max_samples_feature"),
						getString(
								"_UI_PropertyDescriptor_description",
								"_UI_DurabilityServiceQosPolicy_max_samples_feature",
								"_UI_DurabilityServiceQosPolicy_type"),
						ModelPackage.Literals.DURABILITY_SERVICE_QOS_POLICY__MAX_SAMPLES,
						true, false, false,
						ItemPropertyDescriptor.INTEGRAL_VALUE_IMAGE, null, null));
	}

	/**
	 * This adds a property descriptor for the Max samples per instance feature.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	protected void addMax_samples_per_instancePropertyDescriptor(Object object) {
		itemPropertyDescriptors
				.add(createItemPropertyDescriptor(
						((ComposeableAdapterFactory) adapterFactory)
								.getRootAdapterFactory(),
						getResourceLocator(),
						getString("_UI_DurabilityServiceQosPolicy_max_samples_per_instance_feature"),
						getString(
								"_UI_PropertyDescriptor_description",
								"_UI_DurabilityServiceQosPolicy_max_samples_per_instance_feature",
								"_UI_DurabilityServiceQosPolicy_type"),
						ModelPackage.Literals.DURABILITY_SERVICE_QOS_POLICY__MAX_SAMPLES_PER_INSTANCE,
						true, false, false,
						ItemPropertyDescriptor.INTEGRAL_VALUE_IMAGE, null, null));
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
					.add(ModelPackage.Literals.DURABILITY_SERVICE_QOS_POLICY__SERVICE_CLEANUP_DELAY);
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
	 * This returns DurabilityServiceQosPolicy.gif.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	@Override
	public Object getImage(Object object) {
		return overlayImage(object, getResourceLocator().getImage(
				"full/obj16/DurabilityServiceQosPolicy"));
	}

	/**
	 * This returns the label text for the adapted class.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	@Override
	public String getText(Object object) {
		DurabilityServiceQosPolicy durabilityServiceQosPolicy = (DurabilityServiceQosPolicy) object;
		return getString("_UI_DurabilityServiceQosPolicy_type") + " "
				+ durabilityServiceQosPolicy.getHistory_depth();
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

		switch (notification.getFeatureID(DurabilityServiceQosPolicy.class)) {
		case ModelPackage.DURABILITY_SERVICE_QOS_POLICY__HISTORY_DEPTH:
		case ModelPackage.DURABILITY_SERVICE_QOS_POLICY__HISTORY_KIND:
		case ModelPackage.DURABILITY_SERVICE_QOS_POLICY__MAX_INSTANCES:
		case ModelPackage.DURABILITY_SERVICE_QOS_POLICY__MAX_SAMPLES:
		case ModelPackage.DURABILITY_SERVICE_QOS_POLICY__MAX_SAMPLES_PER_INSTANCE:
			fireNotifyChanged(new ViewerNotification(notification, notification
					.getNotifier(), false, true));
			return;
		case ModelPackage.DURABILITY_SERVICE_QOS_POLICY__SERVICE_CLEANUP_DELAY:
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

		newChildDescriptors
				.add(createChildParameter(
						ModelPackage.Literals.DURABILITY_SERVICE_QOS_POLICY__SERVICE_CLEANUP_DELAY,
						ModelFactory.eINSTANCE.createPeriod()));
	}

}
