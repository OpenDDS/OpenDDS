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
 * This is the item provider adapter for a {@link OpenDDS.ResourceLimitsQosPolicy} object.
 * <!-- begin-user-doc -->
 * <!-- end-user-doc -->
 * @generated
 */
public class ResourceLimitsQosPolicyItemProvider extends QosPolicyItemProvider implements IEditingDomainItemProvider,
        IStructuredItemContentProvider, ITreeItemContentProvider, IItemLabelProvider, IItemPropertySource {
    /**
     * This constructs an instance from a factory and a notifier.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public ResourceLimitsQosPolicyItemProvider(AdapterFactory adapterFactory) {
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

            addMax_instancesPropertyDescriptor(object);
            addMax_samplesPropertyDescriptor(object);
            addMax_samples_per_instancePropertyDescriptor(object);
        }
        return itemPropertyDescriptors;
    }

    /**
     * This adds a property descriptor for the Max instances feature.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    protected void addMax_instancesPropertyDescriptor(Object object) {
        itemPropertyDescriptors.add(createItemPropertyDescriptor(((ComposeableAdapterFactory) adapterFactory)
                .getRootAdapterFactory(), getResourceLocator(),
                getString("_UI_ResourceLimitsQosPolicy_max_instances_feature"), getString(
                        "_UI_PropertyDescriptor_description", "_UI_ResourceLimitsQosPolicy_max_instances_feature",
                        "_UI_ResourceLimitsQosPolicy_type"),
                OpenDDSPackage.Literals.RESOURCE_LIMITS_QOS_POLICY__MAX_INSTANCES, true, false, false,
                ItemPropertyDescriptor.INTEGRAL_VALUE_IMAGE, null, null));
    }

    /**
     * This adds a property descriptor for the Max samples feature.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    protected void addMax_samplesPropertyDescriptor(Object object) {
        itemPropertyDescriptors.add(createItemPropertyDescriptor(((ComposeableAdapterFactory) adapterFactory)
                .getRootAdapterFactory(), getResourceLocator(),
                getString("_UI_ResourceLimitsQosPolicy_max_samples_feature"), getString(
                        "_UI_PropertyDescriptor_description", "_UI_ResourceLimitsQosPolicy_max_samples_feature",
                        "_UI_ResourceLimitsQosPolicy_type"),
                OpenDDSPackage.Literals.RESOURCE_LIMITS_QOS_POLICY__MAX_SAMPLES, true, false, false,
                ItemPropertyDescriptor.INTEGRAL_VALUE_IMAGE, null, null));
    }

    /**
     * This adds a property descriptor for the Max samples per instance feature.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    protected void addMax_samples_per_instancePropertyDescriptor(Object object) {
        itemPropertyDescriptors.add(createItemPropertyDescriptor(((ComposeableAdapterFactory) adapterFactory)
                .getRootAdapterFactory(), getResourceLocator(),
                getString("_UI_ResourceLimitsQosPolicy_max_samples_per_instance_feature"), getString(
                        "_UI_PropertyDescriptor_description",
                        "_UI_ResourceLimitsQosPolicy_max_samples_per_instance_feature",
                        "_UI_ResourceLimitsQosPolicy_type"),
                OpenDDSPackage.Literals.RESOURCE_LIMITS_QOS_POLICY__MAX_SAMPLES_PER_INSTANCE, true, false, false,
                ItemPropertyDescriptor.INTEGRAL_VALUE_IMAGE, null, null));
    }

    /**
     * This returns ResourceLimitsQosPolicy.gif.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    @Override
    public Object getImage(Object object) {
        return overlayImage(object, getResourceLocator().getImage("full/obj16/ResourceLimitsQosPolicy"));
    }

    /**
     * This returns the label text for the adapted class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    @Override
    public String getText(Object object) {
        ResourceLimitsQosPolicy resourceLimitsQosPolicy = (ResourceLimitsQosPolicy) object;
        return getString("_UI_ResourceLimitsQosPolicy_type") + " " + resourceLimitsQosPolicy.getMax_instances();
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

        switch (notification.getFeatureID(ResourceLimitsQosPolicy.class)) {
            case OpenDDSPackage.RESOURCE_LIMITS_QOS_POLICY__MAX_INSTANCES:
            case OpenDDSPackage.RESOURCE_LIMITS_QOS_POLICY__MAX_SAMPLES:
            case OpenDDSPackage.RESOURCE_LIMITS_QOS_POLICY__MAX_SAMPLES_PER_INSTANCE:
                fireNotifyChanged(new ViewerNotification(notification, notification.getNotifier(), false, true));
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
    protected void collectNewChildDescriptors(Collection<Object> newChildDescriptors, Object object) {
        super.collectNewChildDescriptors(newChildDescriptors, object);
    }

}
