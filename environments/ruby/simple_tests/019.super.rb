class A
  def blah(a, b)
    puts "hi, #{a} and #{b}!"
  end
end

A.new.blah(1, 2)

class B < A
  def blah
    super("what", "stuff")
  end
end
B.new.blah

class C < A
  def blah(a, b)
    super
  end
end
C.new.blah("woop", "zoop")