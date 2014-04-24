require 'erb'

def multiline(data)
  data.strip.gsub(/$(?=.)/m) {"\\"}
end

def render_erb(data)
  template = ERB.new(data)
  template.result(binding)
end

def gen_dir
  File.join($build_dir, 'gen')
end

def lexer_src_file
  'src/parser/lexer.l'
end

def lexer_gen_file
  File.join(gen_dir, 'lexer.c')
end

def parser_src_file
  'src/parser/parser.y'
end

def parser_gen_file
  File.join(gen_dir, 'parser.c')
end

namespace :parser do
  task :gen => [lexer_gen_file, parser_gen_file]

  directory gen_dir

  file lexer_gen_file => [gen_dir, lexer_src_file] do |t|
    sh "flex --header-file=#{lexer_gen_file.ext 'h'} --outfile=#{lexer_gen_file} #{lexer_src_file}"
  end

  file parser_gen_file => [gen_dir, parser_src_file] do |t|
    cp parser_src_file, t.prerequisites[0]
    chdir t.prerequisites[0] do
      sh "lemon -s #{parser_src_file.pathmap '%f'}"
    end
  end
end
