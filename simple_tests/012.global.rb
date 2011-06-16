$global = 1
puts $global

def a
  yield 2
end
a {|$z| }
puts $z