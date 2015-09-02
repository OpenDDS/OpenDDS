#include "ApplicationLevel.h"

#include "AbstractionLayer.h"

#include "ace/streams.h"
#include "ace/OS_NS_stdlib.h"
#include "ace/OS_NS_sys_stat.h"

ApplicationLevel::ApplicationLevel(AbstractionLayer*  abstract,
                                   const ACE_TString& directory,
                                   const ACE_TString& nodename)
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
  ACE_TString diff_name;
  ACE_TString diff_file_name = directory_;

  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("%s received new diff (%d) -> (%d) for file \"%C\" id %d size %d bytes from %C\n"),
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
    diff_name += ACE_TEXT_CHAR_TO_TCHAR(diff.filename.in());

    file_name_ = ACE_TEXT_CHAR_TO_TCHAR(diff.filename.in());
    file_id_ = diff.file_id;

  }
  else // This is not the first file
  {
    generate_diff_filename(diff_name, diff);
  }

  diff_file_name += diff_name;

  write_difference_file(diff_file_name, diff);

  ACE_DEBUG((LM_INFO,
    ACE_TEXT("Received file \"%s\" from %s\n"),
    diff.filename.in(),
    diff.change_source.in()
    ));
}


bool
ApplicationLevel::generate_diff (long size)
{
  bool generate_result = false;

  // Create the FileDiff and set the values
  DistributedContent::FileDiff diff;
  diff.filename = CORBA::string_dup(ACE_TEXT_ALWAYS_CHAR(file_name_.c_str()));
  diff.file_id = file_id_;
  diff.change_source = CORBA::string_dup(ACE_TEXT_ALWAYS_CHAR(nodename_.c_str()));
  diff.previous_version = current_file_version_;
  diff.new_version = current_file_version_ + 1;

  // intialize the data
  diff.difference.length(size);
  for (long cnt = 0; cnt < size; ++cnt)
  {
    diff.difference[cnt] = (CORBA::Octet) (size % 256);
  }

  // write the file
  ACE_TString diffname;
  generate_diff_filename(diffname, diff);
  ACE_TString full_file_name = directory_ + diffname;
  write_difference_file(full_file_name, diff);

  // publish the diff
  if (0 != abstraction_layer_)
  {
    generate_result = abstraction_layer_->send_diff(diff);
  }

  return generate_result;
}


bool
ApplicationLevel::generate_new_file (const ACE_TString& filename, long size)
{
  bool generate_result = false;
  file_name_ = filename;
  DistributedContent::FileDiff diff;
  diff.file_id = ACE_OS::rand() % 32765;
  diff.filename = CORBA::string_dup(ACE_TEXT_ALWAYS_CHAR(file_name_.c_str()));
  diff.change_source = CORBA::string_dup(ACE_TEXT_ALWAYS_CHAR(nodename_.c_str()));
  diff.previous_version = -1;
  diff.new_version = 0;

  // intialize the data
  diff.difference.length(size);
  for (long cnt = 0; cnt < size; ++cnt)
  {
    diff.difference[cnt] = (CORBA::Octet) 0;
  }

  // save the file information
  file_name_ = ACE_TEXT_CHAR_TO_TCHAR(diff.filename.in());
  file_id_ = diff.file_id;

  // write the file
  ACE_TString full_file_name = directory_ + file_name_;
  write_difference_file(full_file_name, diff);

  // publish the diff
  if (0 != abstraction_layer_)
  {
    generate_result = abstraction_layer_->send_diff(diff);
  }

  return generate_result;
}


void
ApplicationLevel::write_difference_file (const ACE_TString& filename,
                                         const DistributedContent::FileDiff& diff)
{


  ofstream output_stream (ACE_TEXT_ALWAYS_CHAR (filename.c_str()), ios::out | ios::trunc | ios::binary);

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
ApplicationLevel::generate_diff_filename(ACE_TString& filename,
                                         const DistributedContent::FileDiff& diff)
{
  ACE_TCHAR outstr[1024];
  ACE_OS::sprintf(outstr,
          ACE_TEXT("diff_%s_%s_%d_%d"),
          ACE_TEXT_CHAR_TO_TCHAR(diff.change_source.in()),
          ACE_TEXT_CHAR_TO_TCHAR(diff.filename.in()),
          diff.previous_version,
          diff.new_version);

  filename += outstr;
}

bool
ApplicationLevel::publish_file (const ACE_TString& filename)
{
  bool publish_result = false;
  file_name_ = filename;
  DistributedContent::FileDiff diff;
  diff.file_id = ACE_OS::rand() % 32765;
  diff.filename = CORBA::string_dup(ACE_TEXT_ALWAYS_CHAR(file_name_.c_str()));
  diff.change_source = CORBA::string_dup(ACE_TEXT_ALWAYS_CHAR(nodename_.c_str()));
  diff.previous_version = -1;
  diff.new_version = 0;

  //std::string  full_file_name = directory_ + file_name_;
  // Expect the files to be in the current directory.
  ACE_TString full_file_name = file_name_;

  ACE_stat file_state;
  if (0 != ACE_OS::stat(full_file_name.c_str(), &file_state))
  {
    ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR - Unable to get state of file \"%s\"\n"), full_file_name.c_str()));
  }
  else
  {
    size_t size = file_state.st_size;

    FILE* file_handle = ACE_OS::fopen(full_file_name.c_str(), ACE_TEXT("rb"));

    if ( NULL == file_handle)
    {
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR - Unable to open file \"%s\"\n"), full_file_name.c_str()));
    }
    else
    {
      // intialize the data
      diff.difference.length(static_cast<CORBA::ULong>(size));

      size_t read_size = fread(diff.difference.get_buffer(false), 1, size, file_handle);
      if (size != read_size)
      {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("ERROR - Read error only read %d out of %d expected bytes from file \"%s\"\n"),
                   read_size, size, full_file_name.c_str() ));
      }
      else
      {
        // save the file information
        file_name_ = ACE_TEXT_CHAR_TO_TCHAR(diff.filename.in());
        file_id_ = diff.file_id;

        // publish the diff
        if (0 != abstraction_layer_)
        {
          publish_result = abstraction_layer_->send_diff(diff);
          if (publish_result)
          {
            ACE_DEBUG((LM_INFO, ACE_TEXT("Published file \"%s\".\n"), diff.filename.in() ));
          }
        }
      } // end of fread

      ACE_OS::fclose(file_handle);
    } // end of file open
  } // end of file stat
  return publish_result;
}
