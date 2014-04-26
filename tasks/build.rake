require_relative 'util'
require 'tmpdir'


def src_files
  FileList['src/**/*.c'].reject do |f|
    f.pathmap('%f') =~ /^_/
  end
end

def gen_src_files
  FileList[File.join $build_dir, 'gen', '*.c']
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

def gen_obj_files
  gen_src_files.map{|f| obj_file f}
end

namespace :build do
  task :shared_lib => ['parser:gen', obj_files, gen_obj_files].flatten do |t|
    sh "cc -shared -o #{shared_lib_file} #{(obj_files + gen_obj_files).join ' '}"
  end

  src_files.each do |src_file|
    file obj_file(src_file) => src_file do |t|
      mkdir_p t.name.pathmap('%d')
      sh "cc -c #{t.prerequisites[0]} -fpic -fmax-errors=4 #{default_cc_options.join ' '} -I#{$build_dir} -ggdb -o #{t.name}"
    end
  end

  gen_src_files.each do |gen_src_file|
    file obj_file(gen_src_file) => gen_src_file do |t|
      mkdir_p t.name.pathmap('%d')
      sh "cc -c #{t.prerequisites[0]} -fpic -fmax-errors=4 #{(default_cc_options - ['-Werror']).join ' '} -I#{$build_dir} -ggdb -o #{t.name}"
    end
  end
end
