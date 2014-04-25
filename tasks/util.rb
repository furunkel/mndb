
def default_cc_options
  File.read(File.join(Rake.original_dir, '.syntastic_c_config')).each_line.map(&:chomp)
end

def root(pwd = Dir.pwd)
  return nil if pwd == '/'
  if File.exists? File.join(pwd, 'Rakefile')
    pwd
  else
    root File.expand_path('..')
  end
end
