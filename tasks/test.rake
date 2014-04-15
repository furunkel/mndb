require_relative 'util'

def test_src_files
  FileList[File.join Rake.original_dir, 'test', '**/*-test.c']
end

def test_bin_file(src_file)
  src_file.pathmap(File.join Rake.original_dir, 'bin', 'test', '%n')
end

namespace :test do
  test_src_files.each do |src_file|
      file test_bin_file(src_file) => [src_file, 'build:shared_lib'] do |t|
      sh "cc #{t.prerequisites[0]} #{shared_lib_file} #{default_cc_options.join ' '} -ggdb -o #{t.name}"
    end
  end

  multitask :build => test_src_files.map{|f| test_bin_file f} do
  end
end
