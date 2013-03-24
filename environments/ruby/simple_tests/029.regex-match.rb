# needed for tagging to work
r = /^([^()#:]+)(\(([^)]+)?\))?:(.*)$/

m = r.match "zoop"
puts m.class

m = r.match "zoop:doop"
puts m[0], m.values_at(1, 3, 4)

m = r.match "zoop(woop):doop!"
puts m[0], m.values_at(1, 3, 4)
