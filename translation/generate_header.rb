require 'translation/typer'

class GenerateHeader
  def initialize(klass, ti)
    @klass = klass
    @info = ti
    @prefix = ""
    @init_args = nil
  end
  
  def klass_name
    @prefix + @klass.name.to_s
  end
  
  def generate_fields(klass)
    fields = []
    sorted = klass.ivars.sort do |a,b|
      a[0].to_s <=> b[0].to_s
    end
    
    sorted.each do |name, type|
      c_type = @info.to_c_instance(type)
      fields << "  #{c_type} #{name};"
    end
    
    return fields
  end
  
  def generate_new
    "struct #{klass_name} *#{klass_name}_new(#{@init_args});\n"
  end
  
  def generate_new_body
    type = "struct #{klass_name}"
    str =  "#{type} *#{klass_name}_new(#{@init_args}) {\n"
    str << "  #{type} *self;\n"
    str << "  self = (#{type}*)malloc(sizeof(#{type}));\n"
    if @init_args
      str << "  #{klass_name}_initialize(self);\n"
    end
    str << "  return self;\n}\n"
    str
  end
  
  attr_accessor :prefix
  
  def generate_struct
    super_fields = []
    sup = @klass.superklass
    while sup and sup != :Object
      obj = @info.classes[sup]
      super_fields << generate_fields(obj)
      sup = obj.superklass
    end
    
    fields = []
    super_fields.reverse.each do |set|
      fields += set
    end
    
    fields += generate_fields(@klass)
    
    "struct #{klass_name} {\n#{fields.join("\n")}\n};\n"
  end
  
  def wrap_function(name, meth, body)
    "#{function_name(name, meth)} {\n  #{body}\n}\n"
  end
  
  def function_name(name, meth)
    c_type = @info.to_c_instance(meth.type)
    c_name = klass_name + "_" + name.to_s
    if meth.args
      args = meth.args.map do |a_name, a_type|
        "#{@info.to_c_instance(a_type)} #{a_name}"
      end
    else
      args = []
    end
    
    if args.size == 0
      str_args = ""
    else
      str_args = ", " + args.join(", ")
    end
    
    if name == :initialize
      @init_args = str_args  
    end
    
    "#{c_type} #{c_name}(struct #{klass_name} *self#{str_args})"
  end
  
  def generate_functions
    out = []
    @klass.defined_methods.each do |name, meth|
      next unless meth.body
      out << "#{function_name(name, meth)};"
    end
    out.join("\n") + "\n"
  end
  
  def generate
    str = generate_struct
    str << "\n"
    str << generate_functions
    str << generate_new
  end
end