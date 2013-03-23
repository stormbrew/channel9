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
    ["atime", "blksize", "blocks", "ctime",
    "dev", "dev_major", "dev_minor",
    "file?", "ftype", "gid", "ino", "mode", "mtime",
    "nlink", "rdev", "size", "uid"].each do |i|
      attr_accessor i.to_sym
    end

    def writable?
      mode & 0200 != 0
    end
    def writable_real?
      mode & 0200 != 0
    end
    def zero?
      size == 0
    end
    def grpowned?
      return false
    end
    def owned?
      return false
    end
    def pipe?
      return mode & 010000 != 0
    end
    def readable?
      return mode & 0400 != 0
    end
    def readable_real?
      return mode & 0400 != 0
    end
    def setgid?
      return mode & 02000 != 0
    end
    def setuid?
      return mode & 04000 != 0
    end
    def socket?
      return mode & 0140000 != 0
    end
    def sticky?
      return mode & 01000 != 0
    end
    def symlink?
      return mode & 0120000 != 0
    end
    def blockdev?
      return mode & 060000 != 0
    end
    def chardev?
      return mode & 020000 != 0
    end
    def directory?
      return mode & 040000 != 0
    end
    def executable?
      return mode & 0400 != 0
    end
    def executable_real?
      return mode & 0400 != 0
    end
    def file?
      return mode & 0100000 != 0
    end
    def ftype
      return mode & 0170000
    end

  end

  def self.stat(name)
    info = $__c9_ffi_struct_stat.call()
    ok = $__c9_ffi_stat.call(name.to_s_prim, info)
    if (ok == 0)
      stat = Stat.new
      stat.atime = Time.new(info.call(:st_atim).call(:tv_sec))
      stat.blksize = info.call(:st_blksize)
      stat.blocks = info.call(:st_blocks)
      stat.ctime = Time.new(info.call(:st_ctim).call(:tv_sec))
      stat.dev = info.call(:st_dev)
      stat.gid = info.call(:st_gid)
      stat.ino = info.call(:st_ino)
      stat.mode = info.call(:st_mode)
      stat.mtime = Time.new(info.call(:st_mtim).call(:tv_sec))
      stat.nlink = info.call(:st_nlink)
      stat.rdev = info.call(:st_rdev)
      stat.size = info.call(:st_size)
      stat.uid = info.call(:st_uid)
      stat
    else
      raise Errno::ENOENT, "No file #{name}"
    end
  end
end
