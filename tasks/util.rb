
def default_cc_options
  File.read(File.join(Rake.original_dir, '.syntastic_c_config')).each_line.map(&:chomp)
end
