def meth(a, b = 99)
  puts a
  puts b
end

def ident(a)
  a
end

def meth2(a, b = "what", c = ident(1))
  puts a
  puts b
  puts c
end

meth(1,2)
meth(2)
meth2(1,2,3)
meth2(1)
meth2(1,2)
meth2(1,2,3)
