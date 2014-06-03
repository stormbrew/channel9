File.open("/tmp/031.fileio.rb.out", "wb") do |f|
  f.puts "hello"
  f.puts "goodbye"
  f.puts "I don't know why."
end
File.open("/tmp/031.fileio.rb.out", "rb") do |f|
  f.each_line do |line|
    print line
  end
end
File.open("/tmp/031.fileio.rb.out", "ab") do |f|
  1000.times do
    f.puts("boom!")
  end
end
File.open("/tmp/031.fileio.rb.out", "rb") do |f|
  puts f.read
end
