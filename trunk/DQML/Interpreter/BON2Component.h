//###############################################################################################################################################
//
//	Meta and Builder Object Network V2.0 for GME
//	BON2Component.h
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

#ifndef BON2Component_h
#define BON2Component_h

#include "BON.h"
#include "BONImpl.h"
#include <ComponentConfig.h>
#include <iostream>
#include <fstream>

namespace BON
{

// MIC CLASS CODE BEGIN
	class DDSQoSVisitor : public Visitor {
	public:
		DDSQoSVisitor ();
		~DDSQoSVisitor ();

	protected :
		virtual void visitAtomImpl (const Atom& atom);
		virtual void visitModelImpl (const Model& model);
		virtual void visitConnectionImpl ( const Connection& connection);

		void processDataReaderQos (const Model& dataReader);
		void processDataWriterQos (const Model& dataWriter);
		void processDataTopicQos (const Model& topic);

		void outputDDSEntityQos (const Model& dds_entity,
							     const std::string &entity_name, // e.g., "DataReader"
							     const std::string &entity_abbrev, // e.g., "DR"
							     const std::string &qos_connection_name, // e.g., "dr_deadline_Connection"
								 const std::string &qos_name, // e.g., "Deadline"
								 const std::map<std::string, std::string> &attribute_map,
								 // e.g., attrib_map["period"] = "datareader.deadline.period="
								 int entity_count,
								 bool &file_opened,
								 std::ofstream &out_file);

	private:
		std::ofstream out_file_;
	};

/*
	class DelayVisitor : public Visitor {
	public:
		int getGlobalMaxDelay();
		int getDelay(BON::Atom src);

	protected :
		virtual void visitAtomImpl( const Atom& atom );
	private:
		std::set<BON::Atom> sources;
	};
*/

// MIC CLASS CODE END

//###############################################################################################################################################
//
// 	C L A S S : BON::Component
//
//###############################################################################################################################################

class Component
{
	//==============================================================
	// IMPLEMENTOR SPECIFIC PART
	// Insert application specific members and method deifinitions here

	//==============================================================
	// BON2 SPECIFIC PART
	// Do not modify anything below

	// Member variables
	public :
		Project 	m_project;
		bool		m_bIsInteractive;

	public:
		Component();
		~Component();

	public:
		void initialize( Project& project );
		void finalize( Project& project );
		void invoke( Project& project, const std::set<FCO>& setModels, long lParam );
		void invokeEx( Project& project, FCO& currentFCO, const std::set<FCO>& setSelectedFCOs, long lParam );
		void objectInvokeEx( Project& project, Object& currentObject, const std::set<Object>& setSelectedObjects, long lParam );
		Util::Variant getParameter( const std::string& strName );
		void setParameter( const std::string& strName, const Util::Variant& varValue );

	#ifdef GME_ADDON
		void globalEventPerformed( globalevent_enum event );
		void objectEventPerformed( Object& object, unsigned long event, VARIANT v );
	#endif
};

}; // namespace BON

#endif // Bon2Component_H