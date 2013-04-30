local target={}
local target.__index=target
local request_factory=require('shoggoth.http_request_factory')
local observable_table=require('fasterpussycat.core.observable_table')
local basic_auth_cred_store=require('fasterpussycat.core.basic_auth_cred_store')
function target:__index(k)
  if target[k] then
    return target[k]
  end
  return target.whiteboard[k]
end



function target:__newindex(k, v)
  target.whiteboard[k]=v
end

function target:set_listener(func)
  target.whiteboard:set_listener(func)
end

function target:enqueue_request(req)
  self.outstanding_requests=self.outstanding_requests+1
  shoggoth.enqueue_request(req)
end

function target.new(t)
  nt={}
  if t.sub(1,4)~="http" then
    t="http://"..t
  end
  nt.outstanding_requests=0
  nt.request_factory=request_factory:new(t)
  nt.request_factory:inject_request_option_function(function(t,method,path,opts)
    local r=nt.basic_auth_credentials:find_by_path(path)
    if r then
      opts["basic_auth"]=r.username..":"..r.password
    end
  end)
  nt.whiteboard=observable_table.new()
  nt.host=nt.request_factory.prototype_request.host
  nt.path=nt.request_factory.prototype_request.path
  nt.basic_auth_credentials=basic_auth_cred_store.new()
  setmetatable(nt, target)
end

return target
