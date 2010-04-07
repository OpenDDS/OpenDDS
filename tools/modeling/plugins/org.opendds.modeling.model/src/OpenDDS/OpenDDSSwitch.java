/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package OpenDDS;

import java.util.List;

import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;

/**
 * <!-- begin-user-doc --> The <b>Switch</b> for the model's
 * inheritance hierarchy. It supports the call
 * {@link #doSwitch(EObject) doSwitch(object)} to invoke the
 * <code>caseXXX</code> method for each class of the model, starting
 * with the actual class of the object and proceeding up the
 * inheritance hierarchy until a non-null result is returned, which is
 * the result of the switch. <!-- end-user-doc -->
 * @see OpenDDS.OpenDDSPackage
 * @generated
 */
public class OpenDDSSwitch<T> {
    /**
     * The cached model package
     * <!-- begin-user-doc --> <!--
     * end-user-doc -->
     * @generated
     */
    protected static OpenDDSPackage modelPackage;

    /**
     * Creates an instance of the switch.
     * <!-- begin-user-doc --> <!--
     * end-user-doc -->
     * @generated
     */
    public OpenDDSSwitch() {
        if (modelPackage == null) {
            modelPackage = OpenDDSPackage.eINSTANCE;
        }
    }

    /**
     * Calls <code>caseXXX</code> for each class of the model until
     * one returns a non null result; it yields that result. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     *
     * @return the first non-null result returned by a
     *         <code>caseXXX</code> call.
     * @generated
     */
    public T doSwitch(EObject theEObject) {
        return doSwitch(theEObject.eClass(), theEObject);
    }

    /**
     * Calls <code>caseXXX</code> for each class of the model until
     * one returns a non null result; it yields that result. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     *
     * @return the first non-null result returned by a
     *         <code>caseXXX</code> call.
     * @generated
     */
    protected T doSwitch(EClass theEClass, EObject theEObject) {
        if (theEClass.eContainer() == modelPackage) {
            return doSwitch(theEClass.getClassifierID(), theEObject);
        } else {
            List<EClass> eSuperTypes = theEClass.getESuperTypes();
            return eSuperTypes.isEmpty() ? defaultCase(theEObject) : doSwitch(eSuperTypes.get(0), theEObject);
        }
    }

    /**
     * Calls <code>caseXXX</code> for each class of the model until
     * one returns a non null result; it yields that result. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     *
     * @return the first non-null result returned by a
     *         <code>caseXXX</code> call.
     * @generated
     */
    protected T doSwitch(int classifierID, EObject theEObject) {
        switch (classifierID) {
            case OpenDDSPackage.ENTITY: {
                Entity entity = (Entity) theEObject;
                T result = caseEntity(entity);
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.NAMED_ENTITY: {
                NamedEntity namedEntity = (NamedEntity) theEObject;
                T result = caseNamedEntity(namedEntity);
                if (result == null) {
                    result = caseEntity(namedEntity);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.SPECIFICATION: {
                Specification specification = (Specification) theEObject;
                T result = caseSpecification(specification);
                if (result == null) {
                    result = caseEntity(specification);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.DOMAIN_ENTITY: {
                DomainEntity domainEntity = (DomainEntity) theEObject;
                T result = caseDomainEntity(domainEntity);
                if (result == null) {
                    result = caseNamedEntity(domainEntity);
                }
                if (result == null) {
                    result = caseEntity(domainEntity);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.CONTENT_FILTERED_TOPIC: {
                ContentFilteredTopic contentFilteredTopic = (ContentFilteredTopic) theEObject;
                T result = caseContentFilteredTopic(contentFilteredTopic);
                if (result == null) {
                    result = caseTopicDescription(contentFilteredTopic);
                }
                if (result == null) {
                    result = caseDomainEntity(contentFilteredTopic);
                }
                if (result == null) {
                    result = caseModelEntity(contentFilteredTopic);
                }
                if (result == null) {
                    result = caseNamedEntity(contentFilteredTopic);
                }
                if (result == null) {
                    result = caseEntity(contentFilteredTopic);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.MULTI_TOPIC: {
                MultiTopic multiTopic = (MultiTopic) theEObject;
                T result = caseMultiTopic(multiTopic);
                if (result == null) {
                    result = caseTopicDescription(multiTopic);
                }
                if (result == null) {
                    result = caseDomainEntity(multiTopic);
                }
                if (result == null) {
                    result = caseModelEntity(multiTopic);
                }
                if (result == null) {
                    result = caseNamedEntity(multiTopic);
                }
                if (result == null) {
                    result = caseEntity(multiTopic);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.TOPIC: {
                Topic topic = (Topic) theEObject;
                T result = caseTopic(topic);
                if (result == null) {
                    result = caseTopicDescription(topic);
                }
                if (result == null) {
                    result = caseDomainEntity(topic);
                }
                if (result == null) {
                    result = caseModelEntity(topic);
                }
                if (result == null) {
                    result = caseNamedEntity(topic);
                }
                if (result == null) {
                    result = caseEntity(topic);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.TOPIC_DESCRIPTION: {
                TopicDescription topicDescription = (TopicDescription) theEObject;
                T result = caseTopicDescription(topicDescription);
                if (result == null) {
                    result = caseDomainEntity(topicDescription);
                }
                if (result == null) {
                    result = caseModelEntity(topicDescription);
                }
                if (result == null) {
                    result = caseNamedEntity(topicDescription);
                }
                if (result == null) {
                    result = caseEntity(topicDescription);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.ARRAY: {
                Array array = (Array) theEObject;
                T result = caseArray(array);
                if (result == null) {
                    result = caseCollection(array);
                }
                if (result == null) {
                    result = caseTopicField(array);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.OBOOLEAN: {
                OBoolean oBoolean = (OBoolean) theEObject;
                T result = caseOBoolean(oBoolean);
                if (result == null) {
                    result = caseSimple(oBoolean);
                }
                if (result == null) {
                    result = caseTopicField(oBoolean);
                }
                if (result == null) {
                    result = caseKeyField(oBoolean);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.CASE: {
                Case case_ = (Case) theEObject;
                T result = caseCase(case_);
                if (result == null) {
                    result = caseTopicField(case_);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.OCHAR: {
                OChar oChar = (OChar) theEObject;
                T result = caseOChar(oChar);
                if (result == null) {
                    result = caseSimple(oChar);
                }
                if (result == null) {
                    result = caseTopicField(oChar);
                }
                if (result == null) {
                    result = caseKeyField(oChar);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.COLLECTION: {
                Collection collection = (Collection) theEObject;
                T result = caseCollection(collection);
                if (result == null) {
                    result = caseTopicField(collection);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.CONSTRUCTED_TOPIC_TYPE: {
                ConstructedTopicType constructedTopicType = (ConstructedTopicType) theEObject;
                T result = caseConstructedTopicType(constructedTopicType);
                if (result == null) {
                    result = caseTopicField(constructedTopicType);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.ODOUBLE: {
                ODouble oDouble = (ODouble) theEObject;
                T result = caseODouble(oDouble);
                if (result == null) {
                    result = caseSimple(oDouble);
                }
                if (result == null) {
                    result = caseTopicField(oDouble);
                }
                if (result == null) {
                    result = caseKeyField(oDouble);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.ENUM: {
                Enum enum_ = (Enum) theEObject;
                T result = caseEnum(enum_);
                if (result == null) {
                    result = caseTopicField(enum_);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.OFLOAT: {
                OFloat oFloat = (OFloat) theEObject;
                T result = caseOFloat(oFloat);
                if (result == null) {
                    result = caseSimple(oFloat);
                }
                if (result == null) {
                    result = caseTopicField(oFloat);
                }
                if (result == null) {
                    result = caseKeyField(oFloat);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.KEY: {
                Key key = (Key) theEObject;
                T result = caseKey(key);
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.KEY_FIELD: {
                KeyField keyField = (KeyField) theEObject;
                T result = caseKeyField(keyField);
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.OLONG: {
                OLong oLong = (OLong) theEObject;
                T result = caseOLong(oLong);
                if (result == null) {
                    result = caseSimple(oLong);
                }
                if (result == null) {
                    result = caseTopicField(oLong);
                }
                if (result == null) {
                    result = caseKeyField(oLong);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.OLONG_LONG: {
                OLongLong oLongLong = (OLongLong) theEObject;
                T result = caseOLongLong(oLongLong);
                if (result == null) {
                    result = caseSimple(oLongLong);
                }
                if (result == null) {
                    result = caseTopicField(oLongLong);
                }
                if (result == null) {
                    result = caseKeyField(oLongLong);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.OOCTET: {
                OOctet oOctet = (OOctet) theEObject;
                T result = caseOOctet(oOctet);
                if (result == null) {
                    result = caseSimple(oOctet);
                }
                if (result == null) {
                    result = caseTopicField(oOctet);
                }
                if (result == null) {
                    result = caseKeyField(oOctet);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.SEQUENCE: {
                Sequence sequence = (Sequence) theEObject;
                T result = caseSequence(sequence);
                if (result == null) {
                    result = caseCollection(sequence);
                }
                if (result == null) {
                    result = caseTopicField(sequence);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.OSHORT: {
                OShort oShort = (OShort) theEObject;
                T result = caseOShort(oShort);
                if (result == null) {
                    result = caseSimple(oShort);
                }
                if (result == null) {
                    result = caseTopicField(oShort);
                }
                if (result == null) {
                    result = caseKeyField(oShort);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.SIMPLE: {
                Simple simple = (Simple) theEObject;
                T result = caseSimple(simple);
                if (result == null) {
                    result = caseTopicField(simple);
                }
                if (result == null) {
                    result = caseKeyField(simple);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.OSTRING: {
                OString oString = (OString) theEObject;
                T result = caseOString(oString);
                if (result == null) {
                    result = caseSimple(oString);
                }
                if (result == null) {
                    result = caseTopicField(oString);
                }
                if (result == null) {
                    result = caseKeyField(oString);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.TOPIC_STRUCT: {
                TopicStruct topicStruct = (TopicStruct) theEObject;
                T result = caseTopicStruct(topicStruct);
                if (result == null) {
                    result = caseConstructedTopicType(topicStruct);
                }
                if (result == null) {
                    result = caseModelEntity(topicStruct);
                }
                if (result == null) {
                    result = caseTopicField(topicStruct);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.TOPIC_FIELD: {
                TopicField topicField = (TopicField) theEObject;
                T result = caseTopicField(topicField);
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.TYPEDEF: {
                Typedef typedef = (Typedef) theEObject;
                T result = caseTypedef(typedef);
                if (result == null) {
                    result = caseTopicField(typedef);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.OU_LONG: {
                OULong ouLong = (OULong) theEObject;
                T result = caseOULong(ouLong);
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.OU_LONG_LONG: {
                OULongLong ouLongLong = (OULongLong) theEObject;
                T result = caseOULongLong(ouLongLong);
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.UNION: {
                Union union = (Union) theEObject;
                T result = caseUnion(union);
                if (result == null) {
                    result = caseConstructedTopicType(union);
                }
                if (result == null) {
                    result = caseTopicField(union);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.OU_SHORT: {
                OUShort ouShort = (OUShort) theEObject;
                T result = caseOUShort(ouShort);
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.DATA_READER: {
                DataReader dataReader = (DataReader) theEObject;
                T result = caseDataReader(dataReader);
                if (result == null) {
                    result = caseDataReaderWriter(dataReader);
                }
                if (result == null) {
                    result = caseDomainEntity(dataReader);
                }
                if (result == null) {
                    result = caseNamedEntity(dataReader);
                }
                if (result == null) {
                    result = caseEntity(dataReader);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.DATA_READER_WRITER: {
                DataReaderWriter dataReaderWriter = (DataReaderWriter) theEObject;
                T result = caseDataReaderWriter(dataReaderWriter);
                if (result == null) {
                    result = caseDomainEntity(dataReaderWriter);
                }
                if (result == null) {
                    result = caseNamedEntity(dataReaderWriter);
                }
                if (result == null) {
                    result = caseEntity(dataReaderWriter);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.DATA_WRITER: {
                DataWriter dataWriter = (DataWriter) theEObject;
                T result = caseDataWriter(dataWriter);
                if (result == null) {
                    result = caseDataReaderWriter(dataWriter);
                }
                if (result == null) {
                    result = caseDomainEntity(dataWriter);
                }
                if (result == null) {
                    result = caseNamedEntity(dataWriter);
                }
                if (result == null) {
                    result = caseEntity(dataWriter);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.DOMAIN: {
                Domain domain = (Domain) theEObject;
                T result = caseDomain(domain);
                if (result == null) {
                    result = caseNamedEntity(domain);
                }
                if (result == null) {
                    result = caseModelEntity(domain);
                }
                if (result == null) {
                    result = caseEntity(domain);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.DOMAIN_PARTICIPANT: {
                DomainParticipant domainParticipant = (DomainParticipant) theEObject;
                T result = caseDomainParticipant(domainParticipant);
                if (result == null) {
                    result = caseDomainEntity(domainParticipant);
                }
                if (result == null) {
                    result = caseModelEntity(domainParticipant);
                }
                if (result == null) {
                    result = caseNamedEntity(domainParticipant);
                }
                if (result == null) {
                    result = caseEntity(domainParticipant);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.PUBLISHER: {
                Publisher publisher = (Publisher) theEObject;
                T result = casePublisher(publisher);
                if (result == null) {
                    result = casePublisherSubscriber(publisher);
                }
                if (result == null) {
                    result = caseDomainEntity(publisher);
                }
                if (result == null) {
                    result = caseNamedEntity(publisher);
                }
                if (result == null) {
                    result = caseEntity(publisher);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.PUBLISHER_SUBSCRIBER: {
                PublisherSubscriber publisherSubscriber = (PublisherSubscriber) theEObject;
                T result = casePublisherSubscriber(publisherSubscriber);
                if (result == null) {
                    result = caseDomainEntity(publisherSubscriber);
                }
                if (result == null) {
                    result = caseNamedEntity(publisherSubscriber);
                }
                if (result == null) {
                    result = caseEntity(publisherSubscriber);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.SUBSCRIBER: {
                Subscriber subscriber = (Subscriber) theEObject;
                T result = caseSubscriber(subscriber);
                if (result == null) {
                    result = casePublisherSubscriber(subscriber);
                }
                if (result == null) {
                    result = caseDomainEntity(subscriber);
                }
                if (result == null) {
                    result = caseNamedEntity(subscriber);
                }
                if (result == null) {
                    result = caseEntity(subscriber);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.DEADLINE_QOS_POLICY: {
                DeadlineQosPolicy deadlineQosPolicy = (DeadlineQosPolicy) theEObject;
                T result = caseDeadlineQosPolicy(deadlineQosPolicy);
                if (result == null) {
                    result = caseQosPolicy(deadlineQosPolicy);
                }
                if (result == null) {
                    result = caseSpecification(deadlineQosPolicy);
                }
                if (result == null) {
                    result = caseModelEntity(deadlineQosPolicy);
                }
                if (result == null) {
                    result = caseEntity(deadlineQosPolicy);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.DESTINATION_ORDER_QOS_POLICY: {
                DestinationOrderQosPolicy destinationOrderQosPolicy = (DestinationOrderQosPolicy) theEObject;
                T result = caseDestinationOrderQosPolicy(destinationOrderQosPolicy);
                if (result == null) {
                    result = caseQosPolicy(destinationOrderQosPolicy);
                }
                if (result == null) {
                    result = caseSpecification(destinationOrderQosPolicy);
                }
                if (result == null) {
                    result = caseModelEntity(destinationOrderQosPolicy);
                }
                if (result == null) {
                    result = caseEntity(destinationOrderQosPolicy);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.DURABILITY_QOS_POLICY: {
                DurabilityQosPolicy durabilityQosPolicy = (DurabilityQosPolicy) theEObject;
                T result = caseDurabilityQosPolicy(durabilityQosPolicy);
                if (result == null) {
                    result = caseQosPolicy(durabilityQosPolicy);
                }
                if (result == null) {
                    result = caseSpecification(durabilityQosPolicy);
                }
                if (result == null) {
                    result = caseModelEntity(durabilityQosPolicy);
                }
                if (result == null) {
                    result = caseEntity(durabilityQosPolicy);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.DURABILITY_SERVICE_QOS_POLICY: {
                DurabilityServiceQosPolicy durabilityServiceQosPolicy = (DurabilityServiceQosPolicy) theEObject;
                T result = caseDurabilityServiceQosPolicy(durabilityServiceQosPolicy);
                if (result == null) {
                    result = caseQosPolicy(durabilityServiceQosPolicy);
                }
                if (result == null) {
                    result = caseSpecification(durabilityServiceQosPolicy);
                }
                if (result == null) {
                    result = caseModelEntity(durabilityServiceQosPolicy);
                }
                if (result == null) {
                    result = caseEntity(durabilityServiceQosPolicy);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.ENTITY_FACTORY_QOS_POLICY: {
                EntityFactoryQosPolicy entityFactoryQosPolicy = (EntityFactoryQosPolicy) theEObject;
                T result = caseEntityFactoryQosPolicy(entityFactoryQosPolicy);
                if (result == null) {
                    result = caseQosPolicy(entityFactoryQosPolicy);
                }
                if (result == null) {
                    result = caseSpecification(entityFactoryQosPolicy);
                }
                if (result == null) {
                    result = caseModelEntity(entityFactoryQosPolicy);
                }
                if (result == null) {
                    result = caseEntity(entityFactoryQosPolicy);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.GROUP_DATA_QOS_POLICY: {
                GroupDataQosPolicy groupDataQosPolicy = (GroupDataQosPolicy) theEObject;
                T result = caseGroupDataQosPolicy(groupDataQosPolicy);
                if (result == null) {
                    result = caseQosPolicy(groupDataQosPolicy);
                }
                if (result == null) {
                    result = caseSpecification(groupDataQosPolicy);
                }
                if (result == null) {
                    result = caseModelEntity(groupDataQosPolicy);
                }
                if (result == null) {
                    result = caseEntity(groupDataQosPolicy);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.HISTORY_QOS_POLICY: {
                HistoryQosPolicy historyQosPolicy = (HistoryQosPolicy) theEObject;
                T result = caseHistoryQosPolicy(historyQosPolicy);
                if (result == null) {
                    result = caseQosPolicy(historyQosPolicy);
                }
                if (result == null) {
                    result = caseSpecification(historyQosPolicy);
                }
                if (result == null) {
                    result = caseModelEntity(historyQosPolicy);
                }
                if (result == null) {
                    result = caseEntity(historyQosPolicy);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.LATENCY_BUDGET_QOS_POLICY: {
                LatencyBudgetQosPolicy latencyBudgetQosPolicy = (LatencyBudgetQosPolicy) theEObject;
                T result = caseLatencyBudgetQosPolicy(latencyBudgetQosPolicy);
                if (result == null) {
                    result = caseQosPolicy(latencyBudgetQosPolicy);
                }
                if (result == null) {
                    result = caseSpecification(latencyBudgetQosPolicy);
                }
                if (result == null) {
                    result = caseModelEntity(latencyBudgetQosPolicy);
                }
                if (result == null) {
                    result = caseEntity(latencyBudgetQosPolicy);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.LIFESPAN_QOS_POLICY: {
                LifespanQosPolicy lifespanQosPolicy = (LifespanQosPolicy) theEObject;
                T result = caseLifespanQosPolicy(lifespanQosPolicy);
                if (result == null) {
                    result = caseQosPolicy(lifespanQosPolicy);
                }
                if (result == null) {
                    result = caseSpecification(lifespanQosPolicy);
                }
                if (result == null) {
                    result = caseModelEntity(lifespanQosPolicy);
                }
                if (result == null) {
                    result = caseEntity(lifespanQosPolicy);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.LIVELINESS_QOS_POLICY: {
                LivelinessQosPolicy livelinessQosPolicy = (LivelinessQosPolicy) theEObject;
                T result = caseLivelinessQosPolicy(livelinessQosPolicy);
                if (result == null) {
                    result = caseQosPolicy(livelinessQosPolicy);
                }
                if (result == null) {
                    result = caseSpecification(livelinessQosPolicy);
                }
                if (result == null) {
                    result = caseModelEntity(livelinessQosPolicy);
                }
                if (result == null) {
                    result = caseEntity(livelinessQosPolicy);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.OWNERSHIP_QOS_POLICY: {
                OwnershipQosPolicy ownershipQosPolicy = (OwnershipQosPolicy) theEObject;
                T result = caseOwnershipQosPolicy(ownershipQosPolicy);
                if (result == null) {
                    result = caseQosPolicy(ownershipQosPolicy);
                }
                if (result == null) {
                    result = caseSpecification(ownershipQosPolicy);
                }
                if (result == null) {
                    result = caseModelEntity(ownershipQosPolicy);
                }
                if (result == null) {
                    result = caseEntity(ownershipQosPolicy);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.OWNERSHIP_STRENGTH_QOS_POLICY: {
                OwnershipStrengthQosPolicy ownershipStrengthQosPolicy = (OwnershipStrengthQosPolicy) theEObject;
                T result = caseOwnershipStrengthQosPolicy(ownershipStrengthQosPolicy);
                if (result == null) {
                    result = caseQosPolicy(ownershipStrengthQosPolicy);
                }
                if (result == null) {
                    result = caseSpecification(ownershipStrengthQosPolicy);
                }
                if (result == null) {
                    result = caseModelEntity(ownershipStrengthQosPolicy);
                }
                if (result == null) {
                    result = caseEntity(ownershipStrengthQosPolicy);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.PARTITION_QOS_POLICY: {
                PartitionQosPolicy partitionQosPolicy = (PartitionQosPolicy) theEObject;
                T result = casePartitionQosPolicy(partitionQosPolicy);
                if (result == null) {
                    result = caseQosPolicy(partitionQosPolicy);
                }
                if (result == null) {
                    result = caseSpecification(partitionQosPolicy);
                }
                if (result == null) {
                    result = caseModelEntity(partitionQosPolicy);
                }
                if (result == null) {
                    result = caseEntity(partitionQosPolicy);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.PRESENTATION_QOS_POLICY: {
                PresentationQosPolicy presentationQosPolicy = (PresentationQosPolicy) theEObject;
                T result = casePresentationQosPolicy(presentationQosPolicy);
                if (result == null) {
                    result = caseQosPolicy(presentationQosPolicy);
                }
                if (result == null) {
                    result = caseSpecification(presentationQosPolicy);
                }
                if (result == null) {
                    result = caseModelEntity(presentationQosPolicy);
                }
                if (result == null) {
                    result = caseEntity(presentationQosPolicy);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.QOS_POLICY: {
                QosPolicy qosPolicy = (QosPolicy) theEObject;
                T result = caseQosPolicy(qosPolicy);
                if (result == null) {
                    result = caseSpecification(qosPolicy);
                }
                if (result == null) {
                    result = caseModelEntity(qosPolicy);
                }
                if (result == null) {
                    result = caseEntity(qosPolicy);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.READER_DATA_LIFECYCLE_QOS_POLICY: {
                ReaderDataLifecycleQosPolicy readerDataLifecycleQosPolicy = (ReaderDataLifecycleQosPolicy) theEObject;
                T result = caseReaderDataLifecycleQosPolicy(readerDataLifecycleQosPolicy);
                if (result == null) {
                    result = caseQosPolicy(readerDataLifecycleQosPolicy);
                }
                if (result == null) {
                    result = caseSpecification(readerDataLifecycleQosPolicy);
                }
                if (result == null) {
                    result = caseModelEntity(readerDataLifecycleQosPolicy);
                }
                if (result == null) {
                    result = caseEntity(readerDataLifecycleQosPolicy);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.RELIABILITY_QOS_POLICY: {
                ReliabilityQosPolicy reliabilityQosPolicy = (ReliabilityQosPolicy) theEObject;
                T result = caseReliabilityQosPolicy(reliabilityQosPolicy);
                if (result == null) {
                    result = caseQosPolicy(reliabilityQosPolicy);
                }
                if (result == null) {
                    result = caseSpecification(reliabilityQosPolicy);
                }
                if (result == null) {
                    result = caseModelEntity(reliabilityQosPolicy);
                }
                if (result == null) {
                    result = caseEntity(reliabilityQosPolicy);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.RESOURCE_LIMITS_QOS_POLICY: {
                ResourceLimitsQosPolicy resourceLimitsQosPolicy = (ResourceLimitsQosPolicy) theEObject;
                T result = caseResourceLimitsQosPolicy(resourceLimitsQosPolicy);
                if (result == null) {
                    result = caseQosPolicy(resourceLimitsQosPolicy);
                }
                if (result == null) {
                    result = caseSpecification(resourceLimitsQosPolicy);
                }
                if (result == null) {
                    result = caseModelEntity(resourceLimitsQosPolicy);
                }
                if (result == null) {
                    result = caseEntity(resourceLimitsQosPolicy);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.TIME_BASED_FILTER_QOS_POLICY: {
                TimeBasedFilterQosPolicy timeBasedFilterQosPolicy = (TimeBasedFilterQosPolicy) theEObject;
                T result = caseTimeBasedFilterQosPolicy(timeBasedFilterQosPolicy);
                if (result == null) {
                    result = caseQosPolicy(timeBasedFilterQosPolicy);
                }
                if (result == null) {
                    result = caseSpecification(timeBasedFilterQosPolicy);
                }
                if (result == null) {
                    result = caseModelEntity(timeBasedFilterQosPolicy);
                }
                if (result == null) {
                    result = caseEntity(timeBasedFilterQosPolicy);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.TOPIC_DATA_QOS_POLICY: {
                TopicDataQosPolicy topicDataQosPolicy = (TopicDataQosPolicy) theEObject;
                T result = caseTopicDataQosPolicy(topicDataQosPolicy);
                if (result == null) {
                    result = caseQosPolicy(topicDataQosPolicy);
                }
                if (result == null) {
                    result = caseSpecification(topicDataQosPolicy);
                }
                if (result == null) {
                    result = caseModelEntity(topicDataQosPolicy);
                }
                if (result == null) {
                    result = caseEntity(topicDataQosPolicy);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.TRANSPORT_PRIORITY_QOS_POLICY: {
                TransportPriorityQosPolicy transportPriorityQosPolicy = (TransportPriorityQosPolicy) theEObject;
                T result = caseTransportPriorityQosPolicy(transportPriorityQosPolicy);
                if (result == null) {
                    result = caseQosPolicy(transportPriorityQosPolicy);
                }
                if (result == null) {
                    result = caseSpecification(transportPriorityQosPolicy);
                }
                if (result == null) {
                    result = caseModelEntity(transportPriorityQosPolicy);
                }
                if (result == null) {
                    result = caseEntity(transportPriorityQosPolicy);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.USER_DATA_QOS_POLICY: {
                UserDataQosPolicy userDataQosPolicy = (UserDataQosPolicy) theEObject;
                T result = caseUserDataQosPolicy(userDataQosPolicy);
                if (result == null) {
                    result = caseQosPolicy(userDataQosPolicy);
                }
                if (result == null) {
                    result = caseSpecification(userDataQosPolicy);
                }
                if (result == null) {
                    result = caseModelEntity(userDataQosPolicy);
                }
                if (result == null) {
                    result = caseEntity(userDataQosPolicy);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.PERIOD: {
                Period period = (Period) theEObject;
                T result = casePeriod(period);
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.WRITER_DATA_LIFECYCLE_QOS_POLICY: {
                WriterDataLifecycleQosPolicy writerDataLifecycleQosPolicy = (WriterDataLifecycleQosPolicy) theEObject;
                T result = caseWriterDataLifecycleQosPolicy(writerDataLifecycleQosPolicy);
                if (result == null) {
                    result = caseQosPolicy(writerDataLifecycleQosPolicy);
                }
                if (result == null) {
                    result = caseSpecification(writerDataLifecycleQosPolicy);
                }
                if (result == null) {
                    result = caseModelEntity(writerDataLifecycleQosPolicy);
                }
                if (result == null) {
                    result = caseEntity(writerDataLifecycleQosPolicy);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.APPLICATION_TARGET: {
                ApplicationTarget applicationTarget = (ApplicationTarget) theEObject;
                T result = caseApplicationTarget(applicationTarget);
                if (result == null) {
                    result = caseEntity(applicationTarget);
                }
                if (result == null) {
                    result = caseModelEntity(applicationTarget);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.TRANSPORT: {
                Transport transport = (Transport) theEObject;
                T result = caseTransport(transport);
                if (result == null) {
                    result = caseEntity(transport);
                }
                if (result == null) {
                    result = caseModelEntity(transport);
                }
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.MODEL: {
                Model model = (Model) theEObject;
                T result = caseModel(model);
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            case OpenDDSPackage.MODEL_ENTITY: {
                ModelEntity modelEntity = (ModelEntity) theEObject;
                T result = caseModelEntity(modelEntity);
                if (result == null) {
                    result = defaultCase(theEObject);
                }
                return result;
            }
            default:
                return defaultCase(theEObject);
        }
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Entity</em>'.
     * <!-- begin-user-doc --> This implementation
     * returns null; returning a non-null result will terminate the
     * switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Entity</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseEntity(Entity object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Named Entity</em>'.
     * <!-- begin-user-doc --> This
     * implementation returns null; returning a non-null result will
     * terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Named Entity</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseNamedEntity(NamedEntity object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Specification</em>'.
     * <!-- begin-user-doc --> This
     * implementation returns null; returning a non-null result will
     * terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Specification</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseSpecification(Specification object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Domain Entity</em>'.
     * <!-- begin-user-doc --> This
     * implementation returns null; returning a non-null result will
     * terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Domain Entity</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseDomainEntity(DomainEntity object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Content Filtered Topic</em>'.
     * <!-- begin-user-doc --> This
     * implementation returns null; returning a non-null result will
     * terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Content Filtered Topic</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseContentFilteredTopic(ContentFilteredTopic object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Multi Topic</em>'.
     * <!-- begin-user-doc --> This
     * implementation returns null; returning a non-null result will
     * terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Multi Topic</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseMultiTopic(MultiTopic object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Topic</em>'.
     * <!-- begin-user-doc --> This implementation
     * returns null; returning a non-null result will terminate the
     * switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Topic</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseTopic(Topic object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Topic Description</em>'.
     * <!-- begin-user-doc --> This
     * implementation returns null; returning a non-null result will
     * terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Topic Description</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseTopicDescription(TopicDescription object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Array</em>'.
     * <!-- begin-user-doc --> This implementation
     * returns null; returning a non-null result will terminate the
     * switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Array</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseArray(Array object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>OBoolean</em>'.
     * <!-- begin-user-doc --> This
     * implementation returns null; returning a non-null result will
     * terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>OBoolean</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseOBoolean(OBoolean object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Case</em>'.
     * <!-- begin-user-doc --> This implementation
     * returns null; returning a non-null result will terminate the
     * switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Case</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseCase(Case object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>OChar</em>'.
     * <!-- begin-user-doc --> This implementation
     * returns null; returning a non-null result will terminate the
     * switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>OChar</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseOChar(OChar object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Collection</em>'.
     * <!-- begin-user-doc --> This
     * implementation returns null; returning a non-null result will
     * terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Collection</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseCollection(Collection object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Constructed Topic Type</em>'.
     * <!-- begin-user-doc --> This
     * implementation returns null; returning a non-null result will
     * terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Constructed Topic Type</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseConstructedTopicType(ConstructedTopicType object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>ODouble</em>'.
     * <!-- begin-user-doc --> This implementation
     * returns null; returning a non-null result will terminate the
     * switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>ODouble</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseODouble(ODouble object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Enum</em>'.
     * <!-- begin-user-doc --> This implementation
     * returns null; returning a non-null result will terminate the
     * switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Enum</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseEnum(Enum object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>OFloat</em>'.
     * <!-- begin-user-doc --> This implementation
     * returns null; returning a non-null result will terminate the
     * switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>OFloat</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseOFloat(OFloat object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Key</em>'.
     * <!-- begin-user-doc --> This implementation
     * returns null; returning a non-null result will terminate the
     * switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Key</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseKey(Key object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Key Field</em>'.
     * <!-- begin-user-doc --> This
     * implementation returns null; returning a non-null result will
     * terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Key Field</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseKeyField(KeyField object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>OLong</em>'.
     * <!-- begin-user-doc --> This implementation
     * returns null; returning a non-null result will terminate the
     * switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>OLong</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseOLong(OLong object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>OLong Long</em>'.
     * <!-- begin-user-doc --> This
     * implementation returns null; returning a non-null result will
     * terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>OLong Long</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseOLongLong(OLongLong object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>OOctet</em>'.
     * <!-- begin-user-doc --> This implementation
     * returns null; returning a non-null result will terminate the
     * switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>OOctet</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseOOctet(OOctet object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Sequence</em>'.
     * <!-- begin-user-doc --> This
     * implementation returns null; returning a non-null result will
     * terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Sequence</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseSequence(Sequence object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>OShort</em>'.
     * <!-- begin-user-doc --> This implementation
     * returns null; returning a non-null result will terminate the
     * switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>OShort</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseOShort(OShort object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Simple</em>'.
     * <!-- begin-user-doc --> This implementation
     * returns null; returning a non-null result will terminate the
     * switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Simple</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseSimple(Simple object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>OString</em>'.
     * <!-- begin-user-doc --> This implementation
     * returns null; returning a non-null result will terminate the
     * switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>OString</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseOString(OString object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Topic Struct</em>'.
     * <!-- begin-user-doc --> This
     * implementation returns null; returning a non-null result will
     * terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Topic Struct</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseTopicStruct(TopicStruct object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Topic Field</em>'.
     * <!-- begin-user-doc --> This
     * implementation returns null; returning a non-null result will
     * terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Topic Field</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseTopicField(TopicField object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Typedef</em>'.
     * <!-- begin-user-doc --> This implementation
     * returns null; returning a non-null result will terminate the
     * switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Typedef</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseTypedef(Typedef object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>OU Long</em>'.
     * <!-- begin-user-doc --> This implementation
     * returns null; returning a non-null result will terminate the
     * switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>OU Long</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseOULong(OULong object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>OU Long Long</em>'.
     * <!-- begin-user-doc --> This
     * implementation returns null; returning a non-null result will
     * terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>OU Long Long</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseOULongLong(OULongLong object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Union</em>'.
     * <!-- begin-user-doc --> This implementation
     * returns null; returning a non-null result will terminate the
     * switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Union</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseUnion(Union object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>OU Short</em>'.
     * <!-- begin-user-doc --> This
     * implementation returns null; returning a non-null result will
     * terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>OU Short</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseOUShort(OUShort object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Data Reader</em>'.
     * <!-- begin-user-doc --> This
     * implementation returns null; returning a non-null result will
     * terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Data Reader</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseDataReader(DataReader object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Data Reader Writer</em>'.
     * <!-- begin-user-doc --> This
     * implementation returns null; returning a non-null result will
     * terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Data Reader Writer</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseDataReaderWriter(DataReaderWriter object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Data Writer</em>'.
     * <!-- begin-user-doc --> This
     * implementation returns null; returning a non-null result will
     * terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Data Writer</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseDataWriter(DataWriter object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Domain</em>'.
     * <!-- begin-user-doc --> This implementation
     * returns null; returning a non-null result will terminate the
     * switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Domain</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseDomain(Domain object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Domain Participant</em>'.
     * <!-- begin-user-doc --> This
     * implementation returns null; returning a non-null result will
     * terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Domain Participant</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseDomainParticipant(DomainParticipant object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Publisher</em>'.
     * <!-- begin-user-doc --> This
     * implementation returns null; returning a non-null result will
     * terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Publisher</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T casePublisher(Publisher object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Publisher Subscriber</em>'.
     * <!-- begin-user-doc --> This
     * implementation returns null; returning a non-null result will
     * terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Publisher Subscriber</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T casePublisherSubscriber(PublisherSubscriber object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Subscriber</em>'.
     * <!-- begin-user-doc --> This
     * implementation returns null; returning a non-null result will
     * terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Subscriber</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseSubscriber(Subscriber object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Deadline Qos Policy</em>'.
     * <!-- begin-user-doc --> This
     * implementation returns null; returning a non-null result will
     * terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Deadline Qos Policy</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseDeadlineQosPolicy(DeadlineQosPolicy object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Destination Order Qos Policy</em>'.
     * <!-- begin-user-doc
     * --> This implementation returns null; returning a non-null
     * result will terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Destination Order Qos Policy</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseDestinationOrderQosPolicy(DestinationOrderQosPolicy object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Durability Qos Policy</em>'.
     * <!-- begin-user-doc --> This
     * implementation returns null; returning a non-null result will
     * terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Durability Qos Policy</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseDurabilityQosPolicy(DurabilityQosPolicy object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Durability Service Qos Policy</em>'.
     * <!-- begin-user-doc
     * --> This implementation returns null; returning a non-null
     * result will terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Durability Service Qos Policy</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseDurabilityServiceQosPolicy(DurabilityServiceQosPolicy object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Entity Factory Qos Policy</em>'.
     * <!-- begin-user-doc -->
     * This implementation returns null; returning a non-null result
     * will terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Entity Factory Qos Policy</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseEntityFactoryQosPolicy(EntityFactoryQosPolicy object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Group Data Qos Policy</em>'.
     * <!-- begin-user-doc --> This
     * implementation returns null; returning a non-null result will
     * terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Group Data Qos Policy</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseGroupDataQosPolicy(GroupDataQosPolicy object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>History Qos Policy</em>'.
     * <!-- begin-user-doc --> This
     * implementation returns null; returning a non-null result will
     * terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>History Qos Policy</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseHistoryQosPolicy(HistoryQosPolicy object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Latency Budget Qos Policy</em>'.
     * <!-- begin-user-doc -->
     * This implementation returns null; returning a non-null result
     * will terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Latency Budget Qos Policy</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseLatencyBudgetQosPolicy(LatencyBudgetQosPolicy object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Lifespan Qos Policy</em>'.
     * <!-- begin-user-doc --> This
     * implementation returns null; returning a non-null result will
     * terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Lifespan Qos Policy</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseLifespanQosPolicy(LifespanQosPolicy object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Liveliness Qos Policy</em>'.
     * <!-- begin-user-doc --> This
     * implementation returns null; returning a non-null result will
     * terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Liveliness Qos Policy</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseLivelinessQosPolicy(LivelinessQosPolicy object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Ownership Qos Policy</em>'.
     * <!-- begin-user-doc --> This
     * implementation returns null; returning a non-null result will
     * terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Ownership Qos Policy</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseOwnershipQosPolicy(OwnershipQosPolicy object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Ownership Strength Qos Policy</em>'.
     * <!-- begin-user-doc
     * --> This implementation returns null; returning a non-null
     * result will terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Ownership Strength Qos Policy</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseOwnershipStrengthQosPolicy(OwnershipStrengthQosPolicy object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Partition Qos Policy</em>'.
     * <!-- begin-user-doc --> This
     * implementation returns null; returning a non-null result will
     * terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Partition Qos Policy</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T casePartitionQosPolicy(PartitionQosPolicy object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Presentation Qos Policy</em>'.
     * <!-- begin-user-doc -->
     * This implementation returns null; returning a non-null result
     * will terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Presentation Qos Policy</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T casePresentationQosPolicy(PresentationQosPolicy object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Qos Policy</em>'.
     * <!-- begin-user-doc --> This
     * implementation returns null; returning a non-null result will
     * terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Qos Policy</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseQosPolicy(QosPolicy object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of
     * '<em>Reader Data Lifecycle Qos Policy</em>'. <!--
     * begin-user-doc --> This implementation returns null; returning
     * a non-null result will terminate the switch. <!-- end-user-doc
     * -->
     *
     * @param object
     *            the target of the switch.
     * @return the result of interpreting the object as an instance of
     *         '<em>Reader Data Lifecycle Qos Policy</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseReaderDataLifecycleQosPolicy(ReaderDataLifecycleQosPolicy object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Reliability Qos Policy</em>'.
     * <!-- begin-user-doc --> This
     * implementation returns null; returning a non-null result will
     * terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Reliability Qos Policy</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseReliabilityQosPolicy(ReliabilityQosPolicy object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Resource Limits Qos Policy</em>'.
     * <!-- begin-user-doc -->
     * This implementation returns null; returning a non-null result
     * will terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Resource Limits Qos Policy</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseResourceLimitsQosPolicy(ResourceLimitsQosPolicy object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Time Based Filter Qos Policy</em>'.
     * <!-- begin-user-doc
     * --> This implementation returns null; returning a non-null
     * result will terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Time Based Filter Qos Policy</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseTimeBasedFilterQosPolicy(TimeBasedFilterQosPolicy object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Topic Data Qos Policy</em>'.
     * <!-- begin-user-doc --> This
     * implementation returns null; returning a non-null result will
     * terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Topic Data Qos Policy</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseTopicDataQosPolicy(TopicDataQosPolicy object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Transport Priority Qos Policy</em>'.
     * <!-- begin-user-doc
     * --> This implementation returns null; returning a non-null
     * result will terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Transport Priority Qos Policy</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseTransportPriorityQosPolicy(TransportPriorityQosPolicy object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>User Data Qos Policy</em>'.
     * <!-- begin-user-doc --> This
     * implementation returns null; returning a non-null result will
     * terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>User Data Qos Policy</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseUserDataQosPolicy(UserDataQosPolicy object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Period</em>'.
     * <!-- begin-user-doc --> This implementation
     * returns null; returning a non-null result will terminate the
     * switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Period</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T casePeriod(Period object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of
     * '<em>Writer Data Lifecycle Qos Policy</em>'. <!--
     * begin-user-doc --> This implementation returns null; returning
     * a non-null result will terminate the switch. <!-- end-user-doc
     * -->
     *
     * @param object
     *            the target of the switch.
     * @return the result of interpreting the object as an instance of
     *         '<em>Writer Data Lifecycle Qos Policy</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseWriterDataLifecycleQosPolicy(WriterDataLifecycleQosPolicy object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Application Target</em>'.
     * <!-- begin-user-doc --> This
     * implementation returns null; returning a non-null result will
     * terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Application Target</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseApplicationTarget(ApplicationTarget object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Transport</em>'.
     * <!-- begin-user-doc --> This
     * implementation returns null; returning a non-null result will
     * terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Transport</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseTransport(Transport object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Model</em>'.
     * <!-- begin-user-doc --> This implementation
     * returns null; returning a non-null result will terminate the
     * switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Model</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseModel(Model object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>Model Entity</em>'.
     * <!-- begin-user-doc --> This
     * implementation returns null; returning a non-null result will
     * terminate the switch. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>Model Entity</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject) doSwitch(EObject)
     * @generated
     */
    public T caseModelEntity(ModelEntity object) {
        return null;
    }

    /**
     * Returns the result of interpreting the object as an instance of '<em>EObject</em>'.
     * <!-- begin-user-doc --> This implementation
     * returns null; returning a non-null result will terminate the
     * switch, but this is the last case anyway. <!-- end-user-doc -->
     * @param object the target of the switch.
     * @return the result of interpreting the object as an instance of '<em>EObject</em>'.
     * @see #doSwitch(org.eclipse.emf.ecore.EObject)
     * @generated
     */
    public T defaultCase(EObject object) {
        return null;
    }

} // OpenDDSSwitch
