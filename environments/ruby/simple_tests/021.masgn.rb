$counter = 1
def put_sep(*args)
  puts("Test: #{$counter}")
  args.each do |a|
    puts(a)
    puts("====")
  end
  $counter += 1
end
# 1
a, b, c = 1, 2, 3
put_sep a,b,c
# 2
a, b, c = [1,2,3]
put_sep a,b,c
# 3
a, b, c = 1
put_sep a,b,c
# 4
a, b, c = 1, 2, 3, 4
put_sep a,b,c

def stuff(*args)
  args
end

# 5
a, b, c = stuff(1,2,3)
put_sep a,b,c
# 6
a, b, c = stuff(1)
put_sep a,b,c
# 7
a, b, c = stuff(1,2,3,4)
put_sep a,b,c

# 8
a, b, *c = 1, 2, 3, 4
put_sep a,b,c, c.class
# 9
a, b, *c = 1, 2, 3
put_sep a,b,c, c.class
# 10
a, b, *c = 1, 2
put_sep a,b,c, c.class
# 11
a, b, *c = 1
put_sep a,b,c, c.class

# 12
a, b, c = 1, 2, *[3, 4]
put_sep a,b,c, c.class
# 13
a, b, c = 1, *[2, 3]
put_sep a,b,c, b.class
# 14
a, b, c = *[1]
put_sep a,b,c, a.class
