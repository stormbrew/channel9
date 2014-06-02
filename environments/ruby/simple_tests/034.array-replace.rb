# From the rdoc on Array#[]=
a = Array.new
a[4] = "4";                 #=> [nil, nil, nil, nil, "4"]
puts a, '---'
a[0, 3] = [ 'a', 'b', 'c' ] #=> ["a", "b", "c", nil, "4"]
puts a, '---'
a[1..2] = [ 1, 2 ]          #=> ["a", 1, 2, nil, "4"]
puts a, '---'
a[0, 2] = "?"               #=> ["?", 2, nil, "4"]
puts a, '---'
a[0..2] = "A"               #=> ["A", "4"]
puts a, '---'
a[-1]   = "Z"               #=> ["A", "Z"]
puts a, '---'
a[1..-1] = nil              #=> ["A"]
puts a, '---'
a[1..-1] = []               #=> ["A"]
puts a, '---'
a[0, 0] = [ 1, 2 ]          #=> [1, 2, "A"]
puts a, '---'
a[3, 0] = "B"               #=> [1, 2, "A", "B"]
puts a, '---'
