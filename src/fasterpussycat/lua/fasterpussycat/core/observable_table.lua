local observable_table = {}

function observable_table:__newindex(k, v)
  local old_value=self._realtable[k]
  if old_value~=v then
    self:alert_listeners(k,v)
  end
  self._realtable[k]=v
end



function observable_table:__index(k)
  if observable_table[k] then
    return observable_table[k]
  end
  return self._realtable[k]
end



function observable_table:add_listener(listener)
  table.insert(self.listeners, listener)
end



function observable_table:set_listener(listener)
  rawset(self,"_listeners",{listener})
end

function observable_table:alert_listeners(k, v)
  for _,listener in ipairs(self._listeners) do
    listener(self,k,v)
  end
end



function observable_table.make_observable(rt)
  ot={}
  ot._listeners={}
  ot._ignore_keys={}
  ot._real_table=rt
  setmetatable(rt,observable_table)
  return rt
end

function observable_table.new()
  local table={}
  return observable_table.make_observable(table)
end

return observable_table



