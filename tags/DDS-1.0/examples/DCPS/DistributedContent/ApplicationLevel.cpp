#include "ApplicationLevel.h"

#include "AbstractionLayer.h"

#include "ace/streams.h"
#include "ace/OS_NS_stdlib.h"

ApplicationLevel::ApplicationLevel(AbstractionLayer*  abstract,
                                   const std::string& directory,
                                   const std::string& nodename)
: abstraction_layer_(abstract)
, directory_(directory)
, nodename_(nodename)
, file_id_(-1)
, current_file_version_(-1)
, file_write_count_(0)
{
  if (0 != abstraction_layer_)
  {
    abstraction_layer_->attach_application(this);
  }
  directory_ += ACE_DIRECTORY_SEPARATOR_STR;
}


ApplicationLevel::~ApplicationLevel()
{
}


void
ApplicationLevel::receive_diff(const DistributedContent::FileDiff& diff)
{
  std::string diff_name("");
  std::string diff_file_name = directory_;

  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("%s received new diff (%d) -> (%d) for file \"%s\" id %d size %d bytes from %s\n"),
    nodename_.c_str(),
    diff.previous_version,
    diff.new_version,
    diff.filename.in(),
    diff.file_id,
    diff.difference.length(),
    diff.change_source.in()
    ));

  
  // Handle the case where the first file is received
  if (-1 == diff.previous_version)
  {
    diff_name += diff.filename.in();

    file_name_ = diff.filename.in();
    file_id_ = diff.file_id;

  }
  else // This is not the first file
  {
    generate_diff_filename(diff_name, diff);
  }

  diff_file_name += diff_name;

  write_difference_file(diff_file_name, diff);

}


bool
ApplicationLevel::generate_diff (long size)
{
  bool generate_result = false;

  // Create the FileDiff and set the values
  DistributedContent::FileDiff diff;
  diff.filename = CORBA::string_dup(file_name_.c_str());
  diff.file_id = file_id_;
  diff.change_source = CORBA::string_dup(nodename_.c_str());
  diff.previous_version = current_file_version_;
  diff.new_version = current_file_version_ + 1;

  // intialize the data
  diff.difference.length(size);
  for (long cnt = 0; cnt < size; ++cnt)
  {
    diff.difference[cnt] = (CORBA::Octet) (size % 256);
  }

  // write the file 
  std::string diffname;
  generate_diff_filename(diffname, diff);
  std::string full_file_name = directory_ + diffname;
  write_difference_file(full_file_name, diff);

  // publish the diff
  if (0 != abstraction_layer_)
  {
    generate_result = abstraction_layer_->send_diff(diff);
  }

  return generate_result;
}


bool
ApplicationLevel::generate_new_file (const std::string& filename, long size)
{
  bool generate_result = false;
  file_name_ = filename;
  DistributedContent::FileDiff diff;
  diff.file_id = ACE_OS::rand() % 32765;
  diff.filename = CORBA::string_dup(file_name_.c_str());
  diff.change_source = CORBA::string_dup(nodename_.c_str());
  diff.previous_version = -1;
  diff.new_version = 0;

  // intialize the data
  diff.difference.length(size);
  for (long cnt = 0; cnt < size; ++cnt)
  {
    diff.difference[cnt] = (CORBA::Octet) 0;
  }

  // save the file information
  file_name_ = diff.filename.in();
  file_id_ = diff.file_id;

  // write the file 
  std::string  full_file_name = directory_ + file_name_;
  write_difference_file(full_file_name, diff);

  // publish the diff
  if (0 != abstraction_layer_)
  {
    generate_result = abstraction_layer_->send_diff(diff);
  }

  return generate_result;
}


void
ApplicationLevel::write_difference_file (const std::string& filename,
                                         const DistributedContent::FileDiff& diff)
{


  ofstream output_stream (filename.c_str(), ios::out | ios::trunc | ios::binary);

  // Check that the stream opened correctly
  if (output_stream.fail ())
  {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("ERROR - Unable to open file %s\n"),
      filename.c_str() ));
  }
  else
  {
    // write the diff to the file
    const char* buffer = (const char*) diff.difference.get_buffer();
    output_stream.write(buffer , diff.difference.length());

    // check that the data was written to the file.
    if (output_stream.bad ())
    {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("ERROR - Unable to write ouput to the file %s\n"),
        filename.c_str() ));
    }
    else
    {
      // write was successful so set the version and increment write count
      current_file_version_ = diff.new_version;
      file_write_count_++;
    }

    // close the file.
    output_stream.close();
  }

}


long
ApplicationLevel::get_write_count()
{
  return file_write_count_;
}


void
ApplicationLevel::generate_diff_filename(std::string& filename,
                                         const DistributedContent::FileDiff& diff)
{
  char outstr[1024];
  sprintf(outstr,
          "diff_%s_%s_%d_%d",
          diff.change_source.in(),
          diff.filename.in(),
          diff.previous_version,
          diff.new_version);

  filename += outstr;
}

