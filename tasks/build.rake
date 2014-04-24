require_relative 'util'
require 'tmpdir'


def src_files
  (FileList['src/**/*.c'] +
   FileList[File.join $build_dir, 'gen', '*.c']).reject do |f|
    f.pathmap('%f') =~ /^_/
  end
end

def obj_file(src_file)
  src_file.pathmap("%{^.*src,#{$build_dir}}X.o")
end

def shared_lib_file
  File.join Rake.original_dir, 'bin', 'lib', 'libmndb.so'
end

def obj_files
 src_files.map{|f| obj_file f}
end

namespace :build do
  multitask :shared_lib => ['parser:gen', obj_files].flatten do |t|
    sh "cc -shared -o #{shared_lib_file} #{t.prerequisites.join ' '}"
  end

  src_files.each do |src_file|
    file obj_file(src_file) => src_file do |t|
      mkdir_p t.name.pathmap('%d')
      sh "cc -c #{t.prerequisites[0]} -fpic -fmax-errors=4 #{default_cc_options.join ' '} -ggdb -o #{t.name}"
    end
  end
end
