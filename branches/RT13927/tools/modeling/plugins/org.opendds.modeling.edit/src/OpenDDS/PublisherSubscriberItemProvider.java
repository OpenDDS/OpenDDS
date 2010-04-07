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

/**
 * This is the item provider adapter for a
 * {@link OpenDDS.PublisherSubscriber} object. <!-- begin-user-doc -->
 * <!-- end-user-doc -->
 * 
 * @generated
 */
public class PublisherSubscriberItemProvider extends DomainEntityItemProvider implements IEditingDomainItemProvider,
        IStructuredItemContentProvider, ITreeItemContentProvider, IItemLabelProvider, IItemPropertySource {
    /**
     * This constructs an instance from a factory and a notifier. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public PublisherSubscriberItemProvider(AdapterFactory adapterFactory) {
        super(adapterFactory);
    }

    /**
     * This returns the property descriptors for the adapted class.
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    @Override
    public List<IItemPropertyDescriptor> getPropertyDescriptors(Object object) {
        if (itemPropertyDescriptors == null) {
            super.getPropertyDescriptors(object);

            addEntity_factoryPropertyDescriptor(object);
            addPresentationPropertyDescriptor(object);
            addGroup_dataPropertyDescriptor(object);
            addPartitionPropertyDescriptor(object);
            addTransportPropertyDescriptor(object);
        }
        return itemPropertyDescriptors;
    }

    /**
     * This adds a property descriptor for the Entity factory feature.
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    protected void addEntity_factoryPropertyDescriptor(Object object) {
        itemPropertyDescriptors.add(createItemPropertyDescriptor(((ComposeableAdapterFactory) adapterFactory)
                .getRootAdapterFactory(), getResourceLocator(),
                getString("_UI_PublisherSubscriber_entity_factory_feature"), getString(
                        "_UI_PropertyDescriptor_description", "_UI_PublisherSubscriber_entity_factory_feature",
                        "_UI_PublisherSubscriber_type"), OpenDDSPackage.Literals.PUBLISHER_SUBSCRIBER__ENTITY_FACTORY,
                true, false, true, null, null, null));
    }

    /**
     * This adds a property descriptor for the Presentation feature.
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    protected void addPresentationPropertyDescriptor(Object object) {
        itemPropertyDescriptors.add(createItemPropertyDescriptor(((ComposeableAdapterFactory) adapterFactory)
                .getRootAdapterFactory(), getResourceLocator(),
                getString("_UI_PublisherSubscriber_presentation_feature"), getString(
                        "_UI_PropertyDescriptor_description", "_UI_PublisherSubscriber_presentation_feature",
                        "_UI_PublisherSubscriber_type"), OpenDDSPackage.Literals.PUBLISHER_SUBSCRIBER__PRESENTATION,
                true, false, true, null, null, null));
    }

    /**
     * This adds a property descriptor for the Group data feature.
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    protected void addGroup_dataPropertyDescriptor(Object object) {
        itemPropertyDescriptors.add(createItemPropertyDescriptor(((ComposeableAdapterFactory) adapterFactory)
                .getRootAdapterFactory(), getResourceLocator(),
                getString("_UI_PublisherSubscriber_group_data_feature"), getString(
                        "_UI_PropertyDescriptor_description", "_UI_PublisherSubscriber_group_data_feature",
                        "_UI_PublisherSubscriber_type"), OpenDDSPackage.Literals.PUBLISHER_SUBSCRIBER__GROUP_DATA,
                true, false, true, null, null, null));
    }

    /**
     * This adds a property descriptor for the Partition feature. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    protected void addPartitionPropertyDescriptor(Object object) {
        itemPropertyDescriptors.add(createItemPropertyDescriptor(((ComposeableAdapterFactory) adapterFactory)
                .getRootAdapterFactory(), getResourceLocator(), getString("_UI_PublisherSubscriber_partition_feature"),
                getString("_UI_PropertyDescriptor_description", "_UI_PublisherSubscriber_partition_feature",
                        "_UI_PublisherSubscriber_type"), OpenDDSPackage.Literals.PUBLISHER_SUBSCRIBER__PARTITION, true,
                false, true, null, null, null));
    }

    /**
     * This adds a property descriptor for the Transport feature. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    protected void addTransportPropertyDescriptor(Object object) {
        itemPropertyDescriptors.add(createItemPropertyDescriptor(((ComposeableAdapterFactory) adapterFactory)
                .getRootAdapterFactory(), getResourceLocator(), getString("_UI_PublisherSubscriber_transport_feature"),
                getString("_UI_PropertyDescriptor_description", "_UI_PublisherSubscriber_transport_feature",
                        "_UI_PublisherSubscriber_type"), OpenDDSPackage.Literals.PUBLISHER_SUBSCRIBER__TRANSPORT, true,
                false, true, null, null, null));
    }

    /**
     * This returns the label text for the adapted class. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    @Override
    public String getText(Object object) {
        String label = ((PublisherSubscriber) object).getName();
        return label == null || label.length() == 0 ? getString("_UI_PublisherSubscriber_type")
                : getString("_UI_PublisherSubscriber_type") + " " + label;
    }

    /**
     * This handles model notifications by calling
     * {@link #updateChildren} to update any cached children and by
     * creating a viewer notification, which it passes to
     * {@link #fireNotifyChanged}. <!-- begin-user-doc --> <!--
     * end-user-doc -->
     * 
     * @generated
     */
    @Override
    public void notifyChanged(Notification notification) {
        updateChildren(notification);
        super.notifyChanged(notification);
    }

    /**
     * This adds {@link org.eclipse.emf.edit.command.CommandParameter}
     * s describing the children that can be created under this
     * object. <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    @Override
    protected void collectNewChildDescriptors(Collection<Object> newChildDescriptors, Object object) {
        super.collectNewChildDescriptors(newChildDescriptors, object);
    }

}
