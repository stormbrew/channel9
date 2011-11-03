class File < IO
  def self.join(*components)
    components.join('/')
  end

  def self.expand_path(path, dir = "")
    path_parts = path.split("/")
    dir_parts = dir.split("/")

    result = []
    (dir_parts + path_parts).each do |dp|
      case dp
      when '.'
      when ''
        # First empty element is a leading /
        result.push(dp) if result.empty?
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

  def self.exist?(name)
    begin
      stat(name)
      return true
    rescue Errno::ENOENT
    end
    false
  end
  def self.exists?(name)
    exist?(name)
  end

  def self.file?(name)
    begin
      stat(name).file?
    rescue Errno::ENOENT
      false
    end
  end
  def self.directory?(name)
    begin
      stat(name).directory?
    rescue Errno::ENOENT
      false
    end
  end

  class Stat
    def initialize(arr)
      @arr = arr
    end

    ["atime", "blksize", "blockdev?", "blocks", "chardev?", "ctime",
    "dev", "dev_major", "dev_minor", "directory?", "executable?", "executable_real?",
    "file?", "ftype", "gid", "grpowned?", "ino", "mode", "mtime",
    "nlink", "owned?", "pipe?", "rdev", "rdev_major", "rdev_minor", "readable?",
    "readable_real?", "setgid?", "setuid?", "size", "socket?", "sticky?", "symlink?",
    "uid", "writable?", "writable_real?", "zero?"].each_with_index do |i, idx|
      define_method(i.to_sym) do
        @arr.at(idx)
      end
    end
  end
    
  def self.stat(name)
    info = Channel9.prim_stat(name.to_s_prim)
    if (info)
      Stat.new(info)
    else
      raise Errno::ENOENT, "No file #{name}"
    end
  end
end