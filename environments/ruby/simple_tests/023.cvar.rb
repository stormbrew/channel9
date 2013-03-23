class X
  @@blah = 1
  puts @@blah

  def do_stuff
    puts @@blah
    @@blah = 2
    puts @@blah
  end
end

X.new.do_stuff

class X
  puts @@blah
end

class Y < X
  puts @@blah
  def do_stuff
    puts @@blah
    @@blah = 3
    puts @@blah
  end
end

Y.new.do_stuff
class X
  puts @@blah
end
class Y
  puts @@blah
end

X.new.do_stuff
class Y
  puts @@blah
end