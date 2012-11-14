require 'ruby_parser'

require 'channel9'
require 'channel9/ruby/compiler'

module Channel9
  module Ruby
    def self.compile_string(type, str, filename = "__eval__", line = 0)
      stream = Channel9::Stream.new
      stream.build do |builder|
        parser = RubyParser.new
        begin
          tree = parser.parse(str, filename)
          tree = s(type.to_sym, tree)
          tree.file = filename
          tree.line = line
          compiler = Channel9::Ruby::Compiler.new(builder)
          compiler.transform(tree)
        rescue Racc::ParseError => e
          puts "parse error in #{filename}: #{e}"
          return nil
        rescue ArgumentError => e
          puts "argument error in #{filename}: #{e}"
          return nil
        rescue SyntaxError => e
          puts "syntax error in #{filename}: #{e}"
          return nil
        rescue NotImplementedError => e
          puts "not implemented error in #{filename}: #{e}"
          return nil
        rescue RegexpError => e
          puts "invalid regex error in #{filename}: #{e}"
          return nil
        end
      end
      return stream
    end

    def self.compile_eval(str, filename = "__eval__")
      return compile_string(:eval, str, filename)
    end

    def self.compile(filename)
      begin
        File.open("#{filename}", "r") do |f|
          return compile_string(:file, f.read, filename)
        end
      rescue Errno::ENOENT, Errno::EISDIR
        return nil
      end
    end
  end
end
