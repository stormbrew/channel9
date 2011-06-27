eval "puts :boom!"
class X
  def initialize
    @a = 1
  end
end
puts X.new.instance_eval("puts @a; @a + 1")
X.class_eval("def doop; @a; end")
puts X.new.doop