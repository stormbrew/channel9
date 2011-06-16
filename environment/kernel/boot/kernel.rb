module Kernel
  def require(name)
    load(name + ".rb")
  end
end