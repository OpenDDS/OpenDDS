/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
#ifndef MONITORDATA_H
#define MONITORDATA_H

namespace DDS { namespace DCPS { class GUID_t; }}

class QString;

namespace Monitor {

class Options;
class MonitorDataModel;
class MonitorTask;

class MonitorData {
  public:
    /// Construct with an IOR only.
    MonitorData( const Options& options, MonitorDataModel* model);

    /// Virtual destructor.
    virtual ~MonitorData();

    /// Disable operation for orderly shutdown.
    void disable();

    /// @name Messages from GUI to DDS.
    /// @{

    /// Establish a binding to a repository.  There can be only one.
    void setRepoIor( const QString& ior);

    /// @}

    /// @name Messages from DDS to GUI.
    /// @{

    /// Add a new data element to the model.
    void addDataElement(
           const DDS::DCPS::GUID_t& parent,
           const DDS::DCPS::GUID_t& id,
           char* value
         );

    /// Update the value of a data element in the model.
    void updateDataElement(
           const DDS::DCPS::GUID_t& id,
           char* value
         );

    /// Remove a data element from the model.
    void removeDataElement( const DDS::DCPS::GUID_t& id);

    /// @}

  private:
    /// Enabled flag.
    bool enabled_;

    /// Configuration information.
    const Options& options_;

    /// The GUI model.
    MonitorDataModel* model_;

    /// The DDS data source.
    MonitorTask* dataSource_;
};

} // End of namespace Monitor

#endif /* MONITORDATA_H */

