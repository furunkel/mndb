require 'fileutils'

class Util < Thor
  include Thor::Actions

  desc "rename", "Automagically renames an entity from NAME to NEW_NAME"
  def rename(name, new_name)

    fail 'dirty working directory' unless system('git diff --quiet') &&
                                          system('git diff --cached --quiet')

    Dir["./**/*.[ch]"].each do |file|
      content = File.read(file)
      renamed_content = content.gsub(/(?<=mndb_)#{name}(?=_|\b)/, new_name)
                               .gsub(/(?<=MNDB_)#{name.upcase}(?=_|\b)/, new_name.upcase)
                               .gsub(/(?<=mndb-)#{name.gsub('_', '-')}(?=_|\b)/, new_name.gsub('_', '-'))
      if content != renamed_content
        File.write file, renamed_content
      end

      new_file = file.gsub(/(?<=mndb\-|\b)#{name}(?=test\-|\b)/, new_name)
      if new_file != file
        run "git mv #{file} #{new_file}"
      end
    end
  end
end
