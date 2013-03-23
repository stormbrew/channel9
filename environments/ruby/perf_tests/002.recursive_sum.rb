
def recursive_sum(n)
  return n if n <= 1
  return n + recursive_sum(n-1)
end

n = 10000
puts "recursive_sum(#{n}) = " + recursive_sum(n)

