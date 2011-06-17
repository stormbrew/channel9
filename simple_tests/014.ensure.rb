# Unwinding handlers via ensure and rescue. Need to be called correctly 
# when passing through appropriate return handlers.

def with_ensure
  begin
    puts "a"
  ensure
    puts "b"
  end
end

with_ensure
puts "-"

def with_multiple_ensure
  begin
    begin
      puts "a"
    ensure
      puts "b"
    end
  ensure
    puts "c"
  end
end

with_multiple_ensure
puts "-"

def with_ensure_and_return
  begin
    return "a"
  ensure
    puts "b"
  end
end

puts with_ensure_and_return
puts "-"

def with_multiple_ensure_and_return
  begin
    begin
      return "a"
    ensure
      puts "b"
    end
  ensure
    puts "c"
  end
end

puts with_multiple_ensure_and_return
puts "-"

def through_block
  begin
    yield
  ensure
    puts "a"
  end
end

def block_method
  through_block do
    return "b"
  end
end
puts block_method