blorp = "what?"
blah = "#{blorp} hello #{blorp} there"
puts blorp
puts blah

class Z
  def to_s
    "boom"
  end
end

puts "x = #{Z.new}"

puts "done!"