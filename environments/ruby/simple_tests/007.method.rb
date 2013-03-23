a = 1

def meth(a, b)
  puts a, b
end

meth(2, 3)
puts a

def ident(i)
  i
end
puts ident(4)

def ret(i)
  return i
  puts "NOOOOooooooooo!"
end
puts(5)

def mret_simple
  return 1,2
end
a, b = mret_simple
puts(a, '==', b)

def mret_splat_single
  return *ident(1)
end
a = mret_splat_single
puts(a, a.class)

def mret_splat_multi
  return *ident([1,2,3])
end

def empty; end
puts empty.class

def String.empty; end
puts String.empty.class
