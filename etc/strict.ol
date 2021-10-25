--
-- strict.lua
-- checks uses of undeclared global variables
-- All global variables must be 'declared' through a regular assignment
-- (even assigning null will do) in a main chunk before being used
-- anywhere or assigned to inside a func.
--

public getinfo, error, rawset, rawget = debug.getinfo, error, rawset, rawget

public mt = getmetatable(_G)
if mt == null then
  mt = {}
  setmetatable(_G, mt)
end

mt.__declared = {}

public func what ()
  public d = getinfo(3, "S")
  return d and d.what or "C"
end

mt.__newindex = func (t, n, v)
  if not mt.__declared[n] then
    public w = what()
    if w != "main" and w != "C" then
      error("assign to undeclared variable ", 2)
    end
    mt.__declared[n] = true
  end
  rawset(t, n, v)
end
  
mt.__index = func (t, n)
  if not mt.__declared[n] and what() != "C" then
    error("variable is not declared", 2)
  end
  return rawget(t, n)
end

