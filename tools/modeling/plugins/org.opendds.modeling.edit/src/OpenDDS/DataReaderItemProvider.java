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
 * This is the item provider adapter for a {@link OpenDDS.DataReader}
 * object. <!-- begin-user-doc --> <!-- end-user-doc -->
 * 
 * @generated
 */
public class DataReaderItemProvider extends DataReaderWriterItemProvider implements IEditingDomainItemProvider,
        IStructuredItemContentProvider, ITreeItemContentProvider, IItemLabelProvider, IItemPropertySource {
    /**
     * This constructs an instance from a factory and a notifier. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public DataReaderItemProvider(AdapterFactory adapterFactory) {
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

            addTopicPropertyDescriptor(object);
            addReader_data_lifecyclePropertyDescriptor(object);
            addTransport_priorityPropertyDescriptor(object);
            addDurability_servicePropertyDescriptor(object);
            addOwnership_strengthPropertyDescriptor(object);
        }
        return itemPropertyDescriptors;
    }

    /**
     * This adds a property descriptor for the Topic feature. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    protected void addTopicPropertyDescriptor(Object object) {
        itemPropertyDescriptors.add(createItemPropertyDescriptor(((ComposeableAdapterFactory) adapterFactory)
                .getRootAdapterFactory(), getResourceLocator(), getString("_UI_DataReader_topic_feature"), getString(
                "_UI_PropertyDescriptor_description", "_UI_DataReader_topic_feature", "_UI_DataReader_type"),
                OpenDDSPackage.Literals.DATA_READER__TOPIC, true, false, true, null, null, null));
    }

    /**
     * This adds a property descriptor for the Reader data lifecycle
     * feature. <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    protected void addReader_data_lifecyclePropertyDescriptor(Object object) {
        itemPropertyDescriptors.add(createItemPropertyDescriptor(((ComposeableAdapterFactory) adapterFactory)
                .getRootAdapterFactory(), getResourceLocator(),
                getString("_UI_DataReader_reader_data_lifecycle_feature"), getString(
                        "_UI_PropertyDescriptor_description", "_UI_DataReader_reader_data_lifecycle_feature",
                        "_UI_DataReader_type"), OpenDDSPackage.Literals.DATA_READER__READER_DATA_LIFECYCLE, true,
                false, true, null, null, null));
    }

    /**
     * This adds a property descriptor for the Transport priority
     * feature. <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    protected void addTransport_priorityPropertyDescriptor(Object object) {
        itemPropertyDescriptors.add(createItemPropertyDescriptor(((ComposeableAdapterFactory) adapterFactory)
                .getRootAdapterFactory(), getResourceLocator(), getString("_UI_DataReader_transport_priority_feature"),
                getString("_UI_PropertyDescriptor_description", "_UI_DataReader_transport_priority_feature",
                        "_UI_DataReader_type"), OpenDDSPackage.Literals.DATA_READER__TRANSPORT_PRIORITY, true, false,
                true, null, null, null));
    }

    /**
     * This adds a property descriptor for the Durability service
     * feature. <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    protected void addDurability_servicePropertyDescriptor(Object object) {
        itemPropertyDescriptors.add(createItemPropertyDescriptor(((ComposeableAdapterFactory) adapterFactory)
                .getRootAdapterFactory(), getResourceLocator(), getString("_UI_DataReader_durability_service_feature"),
                getString("_UI_PropertyDescriptor_description", "_UI_DataReader_durability_service_feature",
                        "_UI_DataReader_type"), OpenDDSPackage.Literals.DATA_READER__DURABILITY_SERVICE, true, false,
                true, null, null, null));
    }

    /**
     * This adds a property descriptor for the Ownership strength
     * feature. <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    protected void addOwnership_strengthPropertyDescriptor(Object object) {
        itemPropertyDescriptors.add(createItemPropertyDescriptor(((ComposeableAdapterFactory) adapterFactory)
                .getRootAdapterFactory(), getResourceLocator(), getString("_UI_DataReader_ownership_strength_feature"),
                getString("_UI_PropertyDescriptor_description", "_UI_DataReader_ownership_strength_feature",
                        "_UI_DataReader_type"), OpenDDSPackage.Literals.DATA_READER__OWNERSHIP_STRENGTH, true, false,
                true, null, null, null));
    }

    /**
     * This returns DataReader.gif. <!-- begin-user-doc --> <!--
     * end-user-doc -->
     * 
     * @generated
     */
    @Override
    public Object getImage(Object object) {
        return overlayImage(object, getResourceLocator().getImage("full/obj16/DataReader"));
    }

    /**
     * This returns the label text for the adapted class. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    @Override
    public String getText(Object object) {
        String label = ((DataReader) object).getName();
        return label == null || label.length() == 0 ? getString("_UI_DataReader_type")
                : getString("_UI_DataReader_type") + " " + label;
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
