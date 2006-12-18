//###############################################################################################################################################
//
//	Meta and Builder Object Network V2.0 for GME
//	BON2Component.cpp
//
//###############################################################################################################################################

/*
	Copyright (c) Vanderbilt University, 2000-2004
	ALL RIGHTS RESERVED

	Vanderbilt University disclaims all warranties with regard to this
	software, including all implied warranties of merchantability
	and fitness.  In no event shall Vanderbilt University be liable for
	any special, indirect or consequential damages or any damages
	whatsoever resulting from loss of use, data or profits, whether
	in an action of contract, negligence or other tortious action,
	arising out of or in connection with the use or performance of
	this software.
*/

#include "stdafx.h"
#include "BON2Component.h"

#include <iosfwd>

namespace BON
{

// MIC CLASS CODE BEGIN
	DDSQoSVisitor::DDSQoSVisitor()
	{
		out_file_.open ("DDS-debug.txt");
	}

	DDSQoSVisitor::~DDSQoSVisitor()
	{
		out_file_.close ();
	}

	void DDSQoSVisitor::visitAtomImpl( const Atom& atom )
	{
		//out_file_ << "ATOM NAME: " << atom->getAtomMeta().name() << std::endl;

		/*
		std::set<Attribute> attributes = atom->getAttributes();

		std::set<Attribute>::iterator iter;
		// Iterate through the attributes outputting the name and value as prescribed by
		//  the DBE QoS file format (in Ming's email).

		// Simple checks
		// Check that only AND and OR gates have multiple inputs - added by jhoffert
		if (atom->getInConnEnds().size() > 1 && 
			!(atom->getAtomMeta().name() == "AND" || atom->getAtomMeta().name() == "OR")) {
				AfxMessageBox(CString(atom->getPath().c_str()) + ": multiple drivers", MB_ICONSTOP | MB_OK );
			}

		// Check for no lines/wires coming in - OK for top level IOBs but nothing else
		if (atom->getInConnEnds().size() < 1 && 
			!(!atom->getParentModel()->getParentModel() && atom->getAtomMeta().name() == "IOB")) {
				AfxMessageBox(CString(atom->getPath().c_str()) + ": no driver", MB_ICONSTOP | MB_OK );
		}

		// Check for no lines/wires going out - OK for top level IOBs but nothing else
		if (atom->getOutConnEnds().size() < 1 && 
			!(!atom->getParentModel()->getParentModel() && atom->getAtomMeta().name() == "IOB")) {
				AfxMessageBox(CString(atom->getPath().c_str()) + ": output is not used", MB_ICONEXCLAMATION | MB_OK );
		}

		// Collect gates & flip-flops
		if (atom->getAtomMeta().name() == "AND" ||
			atom->getAtomMeta().name() == "OR" ||
			atom->getAtomMeta().name() == "NOT" ||
			atom->getAtomMeta().name() == "Register") {
            
			parts.insert(atom->getPath("."));
		}
		*/
	}

	void DDSQoSVisitor::outputDDSEntityQos (const Model& dds_entity,
											const std::string &entity_name, // e.g., "DataReader"
											const std::string &entity_abbrev, // e.g., "DR"
											const std::string &qos_connection_name,
											const std::string &qos_name,
											const std::map<std::string, std::string> &attribute_map,
											int entity_count,
											bool &file_opened,
											std::ofstream &out_file)
	{
		std::multiset<ConnectionEnd> conns = dds_entity->getConnEnds(qos_connection_name);

		// See if there are any connections to the QoS policy specified by qos_connection_name
		if (conns.size() > 0)
		{
			// Check that there is only one QoS policy as there should be
			if (conns.size() > 1)
			{
				std::string errorMsg ("ERROR: Multiple ");
				errorMsg += qos_name + "QoS Policies specified for ";
				errorMsg += entity_name + " " + dds_entity->getName();
				errorMsg += ".\nQoS Policy Ignored";
				AfxMessageBox(errorMsg.c_str ());
			}
			else
			{
				// Get the one and only "qos_name" QoS Policy and print out its attributes
				std::multiset<ConnectionEnd>::const_iterator iter(conns.begin ());
				ConnectionEnd endPt = *iter;

				FCO fco(endPt);
				if (fco)
				{
					// If we make it here then the end is an fco
					std::set<Attribute> attrs = fco->getAttributes ();
					std::set<Attribute>::const_iterator attr_iter(attrs.begin());

					// Open the QoS settings file if needed.
					if (!file_opened)
					{
						file_opened = true;
						std::string filename;

						char cnt_buf [10];
						::sprintf_s (cnt_buf, "%d", entity_count); 
						std::string cnt_str = cnt_buf;
						filename = entity_abbrev + cnt_str + "_" + dds_entity->getName () + ".txt";
						out_file.open(filename.c_str ());
					}

					// Loop through the QoS attributes and write them to the file
					for (; attr_iter != attrs.end (); ++attr_iter)
					{
						Attribute attr = *attr_iter;
						std::string attr_name = attr->getAttributeMeta ().name ();
						std::map<std::string, std::string>::const_iterator map_iter = attribute_map.find (attr_name);
						if (map_iter != attribute_map.end ())
						{
							out_file << map_iter->second << attr->getStringValue () << std::endl;
						}
					}
				}
				else
				{
					AfxMessageBox("ERROR: Not able to retrieve FCO Impl ptr!");
				}
			}
		}
	}

	void DDSQoSVisitor::processDataReaderQos (const Model& dataReader)
	{
		// If there are QoS policies on this DataReader then
		//  create a new output file for this DataReader's QoS policies
		//  giving it a unique name based on the number of DataReaders
		//  seen so far.
		// Get each QoS policy
		// Print out the values of the policy to the file

		static int dr_count = 1;
		const std::string dr_name("DataReader");
		const std::string dr_prefix("DR");
		std::ofstream output_file;
		bool file_opened = false;
		std::map<std::string, std::string> attrib_map;

		// Handle Deadline QoS Policy
		attrib_map.clear ();
		attrib_map["period"] = "datareader.deadline.period=";
		outputDDSEntityQos (dataReader,
							dr_name,
							dr_prefix,
							"dr_deadline_Connection",
							"Deadline",
							attrib_map,
							dr_count,
							file_opened,
							output_file);

		// Handle Destination Order QoS Policy
		attrib_map.clear ();
		attrib_map["dest_order_kind"] = "datareader.destination_order.kind=";
		outputDDSEntityQos (dataReader,
						    dr_name,
						    dr_prefix,
						    "dr_dstOrder_Connection",
						    "Destination Order",
						    attrib_map,
						    dr_count,
						    file_opened,
						    output_file);

		// Handle Durability QoS Policy
		attrib_map.clear ();
		attrib_map["kind"] = "datareader.durability.kind=";
		outputDDSEntityQos (dataReader,
							dr_name,
							dr_prefix,
							"dr_durqos_Connection",
							"Durability",
							attrib_map,
							dr_count,
							file_opened,
							output_file);

		// Handle History QoS Policy
		attrib_map.clear ();
		attrib_map["history_kind"] = "datareader.history.kind=";
		attrib_map["history_depth"] = "datareader.history.depth=";
		outputDDSEntityQos (dataReader,
							dr_name,
							dr_prefix,
							"dr_history_Connection",
							"History",
							attrib_map,
							dr_count,
							file_opened,
							output_file);

		// Handle Latency Budget QoS Policy
		attrib_map.clear ();
		attrib_map["duration"] = "datareader.latency.duration=";
		outputDDSEntityQos (dataReader,
							dr_name,
							dr_prefix,
							"dr_latency_Connection",
							"Latency Budget",
							attrib_map,
							dr_count,
							file_opened,
							output_file);

		// Handle Liveliness QoS Policy
		attrib_map.clear ();
		attrib_map["lease_duration"] = "datareader.liveliness.lease_duration=";
		attrib_map["liveliness_kind"] = "datareader.liveliness.kind=";
		outputDDSEntityQos (dataReader,
							dr_name,
							dr_prefix,
							"dr_liveliness_Connection",
							"Liveliness",
							attrib_map,
							dr_count,
							file_opened,
							output_file);

		// Handle Ownership QoS Policy
		attrib_map.clear ();
		attrib_map["ownership_kind"] = "datareader.ownership.kind=";
		outputDDSEntityQos (dataReader,
							dr_name,
							dr_prefix,
							"dr_ownership_Connection",
							"Ownership",
							attrib_map,
							dr_count,
							file_opened,
							output_file);

		// Handle Reader Data Lifecycle QoS Policy
		attrib_map.clear ();
		attrib_map["autopurge_nowriter_samples_delay"] = "datareader.reader_data_lifecycle.autopurge_nowriter_samples_delay=";
		attrib_map["autopurge_disposed_samples_delay"] = "datareader.reader_data_lifecycle.autopurge_disposed_samples_delay=";
		outputDDSEntityQos (dataReader,
							dr_name,
							dr_prefix,
							"dr_readerdatalifecycle_Connection",
							"Reader Data Lifecycle",
							attrib_map,
							dr_count,
							file_opened,
							output_file);

		// Handle Reliability QoS Policy
		attrib_map.clear ();
		attrib_map["reliability_kind"] = "datareader.reliability.kind=";
		attrib_map["max_blocking_time"] = "datareader.reliability.max_blocking_time=";
		outputDDSEntityQos (dataReader,
							dr_name,
							dr_prefix,
							"dr_reliability_Connection",
							"Reliability",
							attrib_map,
							dr_count,
							file_opened,
							output_file);

		// Handle Resource Limits QoS Policy
		attrib_map.clear ();
		attrib_map["max_samples"] = "datareader.resource_limits.max_samples=";
		attrib_map["max_instances"] = "datareader.resource_limits.max_instances=";
		attrib_map["max_samples_per_instance"] = "datareader.resource_limits.max_samples_per_instance=";
		outputDDSEntityQos (dataReader,
							dr_name,
							dr_prefix,
							"dr_res_Connection",
							"Resource Limits",
							attrib_map,
							dr_count,
							file_opened,
							output_file);

		// Handle Time Based Filter QoS Policy
		attrib_map.clear ();
		attrib_map["minimum_separation"] = "datareader.timebased_filter.min_separation=";
		outputDDSEntityQos (dataReader,
							dr_name,
							dr_prefix,
							"dr_timebased_Connection",
							"Timebased Filter",
							attrib_map,
							dr_count,
							file_opened,
							output_file);

		// Handle User Data QoS Policy
		attrib_map.clear ();
		attrib_map["user_value"] = "datareader.user_data.value=";
		outputDDSEntityQos (dataReader,
							dr_name,
							dr_prefix,
							"dr_userdata_Connection",
							"User Data",
							attrib_map,
							dr_count,
							file_opened,
							output_file);

		if (file_opened)
		{
			++dr_count;
			output_file.close ();
		}
	}

	void DDSQoSVisitor::processDataWriterQos( const Model& dataWriter )
	{
		// If there are QoS policies on this DataWriter then
		//  create a new output file for this DataWriter's QoS policies
		//  giving it a unique name based on the number of DataWriters
		//  seen so far.
		// Get each QoS policy
		// Print out the values of the policy to the file

		static int dw_count = 1;
		const std::string dw_name("DataWriter");
		const std::string dw_prefix("DW");
		std::ofstream output_file;
		bool file_opened = false;
		std::map<std::string, std::string> attrib_map;

		// Handle Deadline QoS Policy
		attrib_map.clear ();
		attrib_map["period"] = "datawriter.deadline.period=";
		outputDDSEntityQos (dataWriter,
						    dw_name,
						    dw_prefix,
						    "dw_deadline_Connection",
						    "Deadline",
						    attrib_map,
						    dw_count,
						    file_opened,
						    output_file);

		// Handle Destination Order QoS Policy
		attrib_map.clear ();
		attrib_map["dest_order_kind"] = "datawriter.destination_order.kind=";
		outputDDSEntityQos (dataWriter,
						    dw_name,
						    dw_prefix,
						    "dw_dstOrder_Connection",
						    "Destination Order",
						    attrib_map,
						    dw_count,
						    file_opened,
						    output_file);

		// Handle Durability QoS Policy
		attrib_map.clear ();
		attrib_map["kind"] = "datawriter.durability.kind=";
		outputDDSEntityQos (dataWriter,
							dw_name,
							dw_prefix,
							"dw_durqos_Connection",
							"Durability",
							attrib_map,
							dw_count,
							file_opened,
							output_file);

		// Handle Durability Service QoS Policy
		attrib_map.clear ();
		attrib_map["max_samples"] = "datawriter.durability_svc.max_samples=";
		attrib_map["service_cleanup_delay"] = "datawriter.durability_svc.service_cleanup_delay=";
		attrib_map["max_samples_per_instance"] = "datawriter.durability_svc.max_samples_per_instance=";
		attrib_map["max_instances"] = "datawriter.durability_svc.max_instances=";
		attrib_map["history_depth"] = "datawriter.durability_svc.history_depth=";
		attrib_map["history_kind"] = "datawriter.durability_svc.history_kind=";
		outputDDSEntityQos (dataWriter,
							dw_name,
							dw_prefix,
							"dw_dursvc_Connection",
							"Durability Service",
							attrib_map,
							dw_count,
							file_opened,
							output_file);

		// Handle History QoS Policy
		attrib_map.clear ();
		attrib_map["history_kind"] = "datawriter.history.kind=";
		attrib_map["history_depth"] = "datawriter.history.depth=";
		outputDDSEntityQos (dataWriter,
							dw_name,
							dw_prefix,
							"dw_history_Connection",
							"History",
							attrib_map,
							dw_count,
							file_opened,
							output_file);

		// Handle Latency Budget QoS Policy
		attrib_map.clear ();
		attrib_map["duration"] = "datawriter.latency.duration=";
		outputDDSEntityQos (dataWriter,
							dw_name,
							dw_prefix,
							"dw_latency_Connection",
							"Latency Budget",
							attrib_map,
							dw_count,
							file_opened,
							output_file);

		// Handle Lifespan QoS Policy
		attrib_map.clear ();
		attrib_map["lifespan_duration"] = "datawriter.lifespan.duration=";
		outputDDSEntityQos (dataWriter,
							dw_name,
							dw_prefix,
							"dw_lifespan_Connection",
							"Lifespan",
							attrib_map,
							dw_count,
							file_opened,
							output_file);

		// Handle Liveliness QoS Policy
		attrib_map.clear ();
		attrib_map["lease_duration"] = "datawriter.liveliness.lease_duration=";
		attrib_map["liveliness_kind"] = "datawriter.liveliness.kind=";
		outputDDSEntityQos (dataWriter,
							dw_name,
							dw_prefix,
							"dw_liveliness_Connection",
							"Liveliness",
							attrib_map,
							dw_count,
							file_opened,
							output_file);

		// Handle Ownership QoS Policy
		attrib_map.clear ();
		attrib_map["ownership_kind"] = "datawriter.ownership.kind=";
		outputDDSEntityQos (dataWriter,
							dw_name,
							dw_prefix,
							"dw_ownership_Connection",
							"Ownership",
							attrib_map,
							dw_count,
							file_opened,
							output_file);

		// Handle Ownership Strength QoS Policy
		attrib_map.clear ();
		attrib_map["ownership_value"] = "datawriter.ownership.value=";
		outputDDSEntityQos (dataWriter,
							dw_name,
							dw_prefix,
							"dw_ownerstrength_Connection",
							"Ownership Strength",
							attrib_map,
							dw_count,
							file_opened,
							output_file);

		// Handle Reliability QoS Policy
		attrib_map.clear ();
		attrib_map["reliability_kind"] = "datawriter.reliability.kind=";
		attrib_map["max_blocking_time"] = "datawriter.reliability.max_blocking_time=";
		outputDDSEntityQos (dataWriter,
							dw_name,
							dw_prefix,
							"dw_reliability_Connection",
							"Reliability",
							attrib_map,
							dw_count,
							file_opened,
							output_file);

		// Handle Resource Limits QoS Policy
		attrib_map.clear ();
		attrib_map["max_samples"] = "datawriter.resource_limits.max_samples=";
		attrib_map["max_instances"] = "datawriter.resource_limits.max_instances=";
		attrib_map["max_samples_per_instance"] = "datawriter.resource_limits.max_samples_per_instance=";
		outputDDSEntityQos (dataWriter,
							dw_name,
							dw_prefix,
							"dw_res_Connection",
							"Resource Limits",
							attrib_map,
							dw_count,
							file_opened,
							output_file);

		// Handle Transport Priority QoS Policy
		attrib_map.clear ();
		attrib_map["transport_value"] = "datawriter.transport_priority.value=";
		outputDDSEntityQos (dataWriter,
							dw_name,
							dw_prefix,
							"dw_transpri_Connection",
							"Transport Priority",
							attrib_map,
							dw_count,
							file_opened,
							output_file);

		// Handle User Data QoS Policy
		attrib_map.clear ();
		attrib_map["user_value"] = "datawriter.user_data.value=";
		outputDDSEntityQos (dataWriter,
							dw_name,
							dw_prefix,
							"dw_userdata_Connection",
							"User Data",
							attrib_map,
							dw_count,
							file_opened,
							output_file);

		// Handle Writer Data Lifecycle QoS Policy
		attrib_map.clear ();
		attrib_map["autodispose_unregistered_instances"] = "datawriter.writer_data_lifecycle.autodispose_unregistered_instances=";
		outputDDSEntityQos (dataWriter,
							dw_name,
							dw_prefix,
							"dw_writerdatalifecycle_Connection",
							"Writer Data Lifecycle",
							attrib_map,
							dw_count,
							file_opened,
							output_file);

		if (file_opened)
		{
			++dw_count;
			output_file.close ();
		}
	}
		
	void DDSQoSVisitor::processDataTopicQos( const Model& topic )
	{
	}

	void DDSQoSVisitor::visitModelImpl( const Model& model )
	{
		//out_file_ << "MODEL NAME: " << model->getModelMeta().name() << std::endl;
		//out_file_ << "DDS Entity Name: " << model->getName() << std::endl;

		if (model->getModelMeta().name() == "DataReader")
		{
			out_file_ << "DDS DataReader Name: " << model->getName() << std::endl;
			processDataReaderQos(model);
			out_file_ << "...Done DDS DataReader Name: " << model->getName() << std::endl;
		}
		else if (model->getModelMeta().name() == "DataWriter")
		{
			out_file_ << "DDS DataWriter Name: " << model->getName() << std::endl;
			processDataWriterQos(model);
			out_file_ << "...Done DDS DataWriter Name: " << model->getName() << std::endl;

			//out_file_ << "DataWriter Name: " << model->getName() << std::endl;
			// If there are QoS policies on this DataWriter then
			// Create a new output file for this DataWriter's QoS policies
			//  giving it a unique name based on the number of DataWriters
			//  seen so far.
			// Get each QoS policy
			// Print out the values of the policy to the file
		}
		else if (model->getModelMeta().name() == "Topic")
		{
			out_file_ << "DDS Topic Name: " << model->getName() << std::endl;
			//processTopicQos(model);
			out_file_ << "...Done DDS Topic Name: " << model->getName() << std::endl;

			//out_file_ << "Topic Name: " << model->getName() << std::endl;
			// If there are QoS policies on this Topic then
			// Create a new output file for this DataWriter's QoS policies
			//  giving it a unique name based on the number of DataWriters
			//  seen so far.
			// Get each QoS policy
			// Print out the values of the policy to the file
		}
	}

	void DDSQoSVisitor::visitConnectionImpl( const Connection& connection )
	{
		/*
		// Collect nets
		std::pair<std::string, std::string> net;
		
		BON::Atom src = connection->getSrc();
		BON::Atom dst = connection->getDst();

		// Get rid of ports (IOBs)
		if (dst->getAtomMeta().name() == "IOB") {
			// Do not collect this net. It will be collected "from the destination"
			return;
		}

		while (src->getAtomMeta().name() == "IOB") {
			std::multiset<BON::ConnectionEnd> srcEnds = src->getInConnEnds();
			std::multiset<BON::ConnectionEnd>::iterator it = srcEnds.begin();
			if (it == srcEnds.end()) {
				return;
			}
			src = *it;
		}

		net.first = src->getPath(".");
		net.second = dst->getPath(".");
		
		nets.insert(net);
		*/
	}
// MIC CLASS CODE END

//###############################################################################################################################################
//
// 	C L A S S : BON::Component
//
//###############################################################################################################################################

Component::Component()
	: m_bIsInteractive( false )
{
}

Component::~Component()
{
	if ( m_project ) {
		m_project->finalizeObjects();
		finalize( m_project );
		m_project = NULL;
	}
}

// ====================================================
// This method is called after all the generic initialization is done
// This should be empty unless application-specific initialization is needed

void Component::initialize( Project& project )
{
	// ======================
	// Insert application specific code here
}

// ====================================================
// This method is called before the whole BON2 project released and disposed
// This should be empty unless application-specific finalization is needed

void Component::finalize( Project& project )
{
	// ======================
	// Insert application specific code here
	AfxMessageBox("DBE Interpreter Finished!");
}

// ====================================================
// This is the obsolete component interface
// This present implementation either tries to call InvokeEx, or does nothing except of a notification

void Component::invoke( Project& project, const std::set<FCO>& setModels, long lParam )
{
	#ifdef SUPPORT_OLD_INVOKE
		Object focus;
		invokeEx( project, focus, setModels, lParam );
	#else
		if ( m_bIsInteractive )
			AfxMessageBox("This BON2 Component does not support the obsolete invoke mechanism!");
	#endif
}

// ====================================================
// This is the main component method for Interpereters and Plugins.
// May also be used in case of invokeable Add-Ons

void Component::invokeEx( Project& project, FCO& currentFCO, const std::set<FCO>& setSelectedFCOs, long lParam )
{
#ifdef GME_ADDON
	project->setAutoCommit( false);
#endif
	// ======================
	// Insert application specific code here

// MIC CLASS CODE BEGIN
	DDSQoSVisitor ddsQosVisitor;
	project->getRootFolder()->accept(&ddsQosVisitor);
// MIC CLASS CODE END
}

// ====================================================
// GME currently does not use this function
// You only need to implement it if other invokation mechanisms are used

void Component::objectInvokeEx( Project& project, Object& currentObject, const std::set<Object>& setSelectedObjects, long lParam )
	{
		if ( m_bIsInteractive )
			AfxMessageBox("This BON2 Component does not support objectInvokeEx method!");
	}

// ====================================================
// Implement application specific parameter-mechanism in these functions

Util::Variant Component::getParameter( const std::string& strName )
{
	// ======================
	// Insert application specific code here

	return Util::Variant();
}

void Component::setParameter( const std::string& strName, const Util::Variant& varValue )
{
	// ======================
	// Insert application specific code here
}

#ifdef GME_ADDON

// ====================================================
// If the component is an Add-On, then this method is called for every Global Event

void Component::globalEventPerformed( globalevent_enum event )
{
	// ======================
	// Insert application specific code here
}

// ====================================================
// If the component is an Add-On, then this method is called for every Object Event

void Component::objectEventPerformed( Object& object, unsigned long event, VARIANT v )
{
	// ======================
	// Insert application specific code here
}

#endif // GME_ADDON

}; // namespace BON

