require_relative 'util'
require 'tmpdir'

$build_dir = Dir.mktmpdir 'mndb_build'

at_exit {
  $stderr.puts 'Cleaning up temporary build directories...'
  FileUtils.remove_entry_secure $build_dir
}

def src_files
  FileList['src/**/*.c']
end

def obj_file(src_file)
  src_file.pathmap("%{^.*src,#{$build_dir}}X.o")
end

def shared_lib_file
  File.join Rake.original_dir, 'bin', 'lib', 'libmndb.so'
end

namespace :build do
  multitask :shared_lib => src_files.map{|f| obj_file f} do |t|
    sh "cc -shared -o #{shared_lib_file} #{t.prerequisites.join ' '}"
  end

  src_files.each do |src_file|
    file obj_file(src_file) => src_file do |t|
      mkdir_p t.name.pathmap('%d')
      sh "cc -c #{t.prerequisites[0]} -fpic #{default_cc_options.join ' '} -o #{t.name}"
    end
  end

  task :obj_files do
  end
end
