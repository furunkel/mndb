require 'tmpdir'
require_relative 'tasks/util'

$build_dir = File.join Dir.tmpdir, '_mndb_build'
$root = root

Dir['tasks/**/*.rake'].each{|r| import r}
