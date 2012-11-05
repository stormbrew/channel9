
SIZE=10000

class Node
  def initialize(val)
    @val = val
    @next = nil
  end

  def next
    @next
  end
  def next=(a)
    @next = a
  end
  def val
    @val
  end
end

puts "creating linked list"
head = Node.new(0)
node = head
i = 0
while i < SIZE
  i += 1
  node.next = Node.new(i)
  node = node.next
end

puts "summing linked list"
node = head
sum = 0
while node
  sum += node.val
  node = node.next
end
puts "sum (0.."+SIZE+") = " + sum


