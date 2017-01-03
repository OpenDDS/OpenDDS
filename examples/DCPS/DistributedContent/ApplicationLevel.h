#ifndef APPLICATIONLEVEL_H_
#define APPLICATIONLEVEL_H_

#include "FileInfoC.h"

#include <ace/String_Base.h>

/// Forward Declarations
class AbstractionLayer;


/**
 * @class ApplicationLevel
 *
 * @brief Provides the actual management of the differences.
 *
 */
class ApplicationLevel
{
public:
  /**
   * Constructor
   *
   * @param abstract - porinter to the Abstraction Layer
   * @param directory - directory where the files will be stored
   * @param nodename - name of this application
   * @return
   */
  ApplicationLevel(AbstractionLayer*  abstract,
                   const ACE_TString& directory,
                   const ACE_TString& nodename);

  virtual ~ApplicationLevel();

  /**
   * Receive a published FileDiff
   *
   * @param diff - the FileDiff
   */
  void receive_diff (const DistributedContent::FileDiff& diff);

  /**
   * Create and publish a new FileDiff for the current file.
   *
   * @param size - size of the diff data.
   *
   * @return  true if the FileDiff is published
   */
  bool generate_diff (long size);

  /**
   * Create and publish a new file.
   *
   * @param filename - name of the new file
   * @param size - size of the new file
   *
   * @return  true if the new file is created and published
   */
  bool generate_new_file (const ACE_TString& filename, long size);

  /**
   * Open and publish a existing file.
   *
   * @param filename - name of the new file
   *
   * @return  true if the new file is opened and published
   */
  bool publish_file (const ACE_TString& filename);

  /**
   * Get the number of files that have been written.
   *
   * @return number of files written
   */
  long get_write_count();

protected:

  /**
   * Write the data in the FileDiff.difference to a file
   *
   * @param filename - name of the file to write
   * @param diff - FileDiff holding the data to write
   */
  void write_difference_file (const ACE_TString& filename,
                              const DistributedContent::FileDiff& diff);

  /**
   * Generate the filename to store the FileDiff information in.
   * @param filename - the filename that will be generated
   * @param diff - FileDiff with the information to be stored
   */
  void generate_diff_filename(ACE_TString& filename,
                              const DistributedContent::FileDiff& diff);

private:
  /// Abstraction Layer that obscures the sending / receiving
  /// on the DDS system.
  AbstractionLayer* abstraction_layer_;
  /// Directory to store the files in
  ACE_TString       directory_;
  /// Name of this node
  ACE_TString       nodename_;

  /// Name of the file that the differences are for
  ACE_TString       file_name_;
  /// Id of the file that the differences are for
  ::CORBA::Long     file_id_;
  /// The latest difference version of the file
  ::CORBA::Long     current_file_version_;
  /// The current number of file writes that have occured.
  ::CORBA::Long     file_write_count_;

};

#endif
