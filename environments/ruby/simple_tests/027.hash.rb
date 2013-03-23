x = {}
x[10] = 2
puts x[10]

x[21] = "blorp"
puts x[21]

x.each {|k, v|
	puts "#{k}=>#{v}"
}
