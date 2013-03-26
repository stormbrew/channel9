class File < IO
  def self.new(filename, mode = "r", opt = nil)
    f = allocate
    f.initialize(filename, mode, opt)
    f
  end

  def self.open(filename, mode = "r", opt = nil, &block)
    f = new(filename, mode, opt)
    if block
      begin
        return block.call(f)
      ensure
        f.close
      end
    end
    f
  end

  def initialize(filename, mode = "r", opt = nil)
    @file_ptr = $__c9_ffi_fopen.fopen(filename.to_s_prim, mode.to_s_prim)
    if @file_ptr == 0
      raise "Error opening #{filename}"
    end
  end

  def close
    if @file_ptr
      $__c9_ffi_fclose.fclose(@file_ptr)
      @file_ptr = nil
    end
  end

  def write(s)
    s = s.to_s_prim
    $__c9_ffi_fwrite.fwrite(s, 1, s.length, @file_ptr)
  end

  def c9_make_buffer
    # yes this is slow and ugly. TODO: Add a real buffer object we
    # can use for this stuff!
    buffer = :"          "
    buffer = buffer + buffer + buffer + buffer + buffer
    buffer = buffer + buffer + buffer + buffer + buffer
    buffer
  end

  def gets
    newline_matcher = %r{\n}
    str = "".to_s_prim
    while true
      buffer = c9_make_buffer
      s = $__c9_ffi_fgets.fgets(buffer, buffer.length, @file_ptr)
      if s != 0
        # didn't hit eof, so figure out how much we actually read. If there's
        # a \n in the result, that's the line done. If not, the buffer wasn't large enough
        # so go again until we find the end of the line.
        if match = newline_matcher.match(buffer)
          str += buffer.substr(0, match.begin(0))
          return String.new(str)
        else
          # cut off the nil character it will have placed at the end.
          str += buffer.substr(0, buffer.length-1)
          # and then let it loop to add more
        end
      elsif str.length > 0
        # if we hit EOF but have already buffered some string, return that.
        return String.new(str)
      else
        # otherwise we're at EOF and should return nil.
        return nil
      end
    end
  end

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
    if path_parts.length > 0
      path_parts.join("/")
    else
      "."
    end
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
