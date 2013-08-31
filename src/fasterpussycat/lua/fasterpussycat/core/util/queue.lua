local queue={}
queue.__index=queue
function queue.new()
  local c={top=1, last=0}
  setmetatable(c,queue)
  return c
end

function queue:push(element)
  local last=self.last+1
  self[last]=element
  self.last=last
end

function queue:pop()
  if self.top>self.last then
    return nil
  end
  local v=self[self.top]
  print(v)
  self[self.top]=nil
  self.top=self.top+1
  return v
end

return queue
