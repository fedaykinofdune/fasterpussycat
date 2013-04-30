local table=require('fasterpussycat.core.table')
local context={}
context.__index=context

function context.new(opts)
  c=table.dup(c.opts)
  
  c.request_factory={}
  setmetatable(c.request_factory, c.target.request_factory)
  if(c.page) then
    c.page=observable_table.make_observable(c.page)
    c.request=c.page.request
    c.request_factory:set_injection_options({referrer=c.request.path, target=c.target})
  else
    c.request_factory:set_injection_options({target=c.target})
  end
  setmetatable(c,context)
  return c
end



function context:set_throw_event_listener(func)
  self.throw_event_listener=func
end



function context:throw_event(event)
  if type(self.throw_event_listener)=="function" then 
    self.throw_event_listener(event)
  end
end

function context:set_table_listener(func)
  self.table_listener=func
  self.target:set_table_listener(func)
  if self.page then
    self.page:set_table_listener(func)
  end
end

function context:collect_events(func, ...)
  local ret={}
  local old_table_listener=self.table_listener
  local old_throw_event_listener=self.throw_event_listener
  self:set_table_listener(function(obj,k,v)
    if self.page and obj==self.page.whiteboard then 
      table.insert(ret,{name="page_key",key=k,value=v})
    elseif obj==self.target.whiteboard then
      table.insert(ret,{name="target_key", key=k, value=v})
    end
  end)
  self:set_throw_event_listener(function(event)
      table.insert(ret,event)
  end)
  func(...)
  self:set_table_listener(old_table_listener)
  self:set_throw_event_listener(old_throw_event_listener)
  return ret
end


function context:sync_get(...)
  return self:sync_req(METHOD_GET , ...)
end



function context:sync_post(...)
  return self:sync_req(METHOD_POST , ...)
end



function context:sync_head(...)
  return self:sync_req(METHOD_HEAD , ...)
end

function context:sync_req(method, ...)
  local co=coroutine.running()
  local page=nil
  req=c.request_factory(method, ...)
  req.on_complete=function(ctx) 
    coroutine.resume(co, ctx.page)
  end
  c.target:enqueue_request(req)
  page=coroutine.yield()
  return page
end

function context:modify_request(req)
  if type(req.on_exists=="function") then
    req.on_complete(function(ctx)
        if ctx.page:exists() then
          req.on_exists(ctx)
        end
    end)
  end
end

function context:get(...)
  local req
  req=c.request_factory.make_new_request(METHOD_GET, ...)
  self:modify_request(req)
  c.target.enqueue_request(req)
end

function context:post(...)
  local req
  req=c.request_factory.make_new_request(METHOD_POST, unpack(arg))
  self:modify_request(req)
  shoggoth.enqueue_request(req)
end


function context:head(...)
  local req
  req=c.request_factory.make_new_request(METHOD_HEAD, unpack(arg))
  self:modify_request(req)
  shoggoth.enqueue_request(req)
end


