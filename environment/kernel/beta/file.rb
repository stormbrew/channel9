class File
  def self.join(a,b)
    "#{a}/#{b}"
  end

  def self.expand_path(path, dir = "")
    path_parts = path.split("/")
    dir_parts = dir.split("/")

    result = []
    (dir_parts + path_parts).each do |dp|
      case dp
      when '.'
      when '..'
        if (result.length > 0)
          result.pop
        else
          result.push(dp)
        end
      else
        result.push(dp)
      end
    end
    result = result.join('/')
    result
  end

  def self.dirname(path)
    path_parts = path.split("/")
    path_parts.pop
    path_parts.join("/")
  end

  def self.exist?
    false # TODO: Make not a stub.
  end
end