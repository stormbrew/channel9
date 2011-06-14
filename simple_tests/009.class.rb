class X < Object
  puts "hi!"
  def stuff(a)
    puts a
  end
end
X.new.stuff(1)