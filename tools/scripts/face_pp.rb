require 'byebug'

$defines = %w(OPENDDS_NO_QUERY_CONDITION
              OPENDDS_NO_CONTENT_FILTERED_TOPIC
              OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
              OPENDDS_NO_MULTI_TOPIC
              OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
              OPENDDS_NO_OWNERSHIP_PROFILE
              OPENDDS_NO_OBJECT_MODEL_PROFILE
              OPENDDS_NO_PERSISTENCE_PROFILE
              OPENDDS_SAFETY_PROFILE)

$cpp_comment = /\/\/.*/
$full_c_comment = /\/\*.*\*\//
$start_c_comment = /(.*)\/\*/
$end_c_comment = /\*\/(.*)/
$cond_false = []
$cond_false_end = [/#endif/, /#else/, /#elif/]

$defines.each do |symbol|
  $cond_false << /#ifndef +\(#{symbol}\)/
  $cond_false << /#ifndef +#{symbol}/
  $cond_false << /#if +!defined +\(#{symbol}\)/
  $cond_false << /#if +!defined +#{symbol}/
  $cond_false << /#elif +!defined +\(#{symbol}\)/
  $cond_false << /#elif +!defined +#{symbol}/
end

$cond_stack = [true]

def cond_eval line
  if false == $cond_stack[-1]
    $cond_false_end.each do |expr|
      if line.match expr
        # Modify last
        $cond_stack[-1] = :pop
        break
      end
    end
  end 
  $cond_false.each do |expr|
    if line.match expr
      $cond_stack.push false
      break
    end
  end
end

def pp_file filepath, outpath
  File.open(outpath, 'w') do |out|
    File.open(filepath) do |file|
      inside_comment = nil
      file.each_line do |line|
        # Strip inline comments
        line.sub!($cpp_comment, '')
        line.sub!($full_c_comment, '')
        if result = line.match($start_c_comment)
          line = line.sub($start_c_comment, result.captures[0])
          out.write result.captures[0]
          inside_comment = true # Won't print line
        elsif inside_comment && (result = line.match($end_c_comment))
          line.sub!($end_c_comment, result.captures[0])
          inside_comment = nil
        end
        unless inside_comment
          cond_eval(line)
          if $cond_stack[-1] == :pop
            $cond_stack.pop
          elsif $cond_stack[-1]
            out.write line
          end
        end
      end
    end
  end
end

$ignore  = ['.', '..', '.shobj']
$formats = ['.h', '.hpp', '.cpp', '.inl']

def pp_dir dirpath, outpath
  Dir.mkdir outpath unless Dir.exists? outpath

  Dir.open(dirpath) do |dir|
    dir.each do |file|
      filepath = [dirpath, file].join(File::SEPARATOR)
      outfilepath = [outpath, file].join(File::SEPARATOR)
      if $ignore.include? file
        # Ignore file
      elsif File.directory? filepath
        subdirpath = [dirpath, file].join(File::SEPARATOR)
        suboutpath = [outpath, file].join(File::SEPARATOR)
        pp_dir subdirpath, suboutpath
      elsif $formats.include? File.extname(file)
        processed_file = pp_file(filepath, outfilepath)
      end
    end
  end
end


PP_OUT_ROOT = ARGV[0] or raise ("did not provide dir")
pp_dir '.', PP_OUT_ROOT
