local queue={}

function queue.new()
  return {top=0, last=-1}
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
  self[self.top]=nil
  self.top=self.top+1
  return v
end
