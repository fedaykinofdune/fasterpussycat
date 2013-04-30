local supervisor={}
local plugin_registry=require("fasterpussycat.core.plugin_registry")
local page_helpers=require("fasterpussycat.core.page_helpers")
local queue=require("fasterpussycat.core.util.queue")
function supervisor:initialize()
  self.events={}
  self.target_queue=queue.new()
  plugin_registry=require("fasterpussycat.core.plugin_registry")
  plugin_registry:load_plugins("/home/caleb/fasterpussycat/src/fasterpussycat/lua/plugins")
  plugin_registry:enable_plugins({})
  self.max_targets=5
  self.current_targets=0
  return elf
end

function supervisor:new_target(target)
  local t=Target.new(target)
  self:target_queue:push(t)
  self:check_target_queue()
end


function supervisor:check_target_queue()
  if self.current_targets<supervisor.current_targets then
    local t=self.target_queue.pop()
    if t then
      local ctx=context:new(target=t)
      self:handle_events({name="new_target", value=t}, ctx)
    end
  end
end

function supervisor:run_plugins(event, ctx, cat)
  local new_events
  local all_events={}
  new_events=context:collect_events(function() 
      runnable=plugin_registry:find_runnable_plugins(event, cat)
      for _,p in ipairs(runnable) do
        p.func(ctx, event)
      end
    end)
   return new_events
end

function supervisor:poll()
  local pages=shoggoth.poll()
  local ctx
  for _,page in ipairs(pages) do
    setmetatable(page,page_helpers)
    ctx=context:new(page=page)
    self:handle_events({name="new_page", value=page}, ctx)
    ctx.target.outstanding_requests=ctx.target.outstanding_requests-1
    if ctx.target.outstanding_requests==0 then
      supervisor.current_targets=supervisor.current_targets-1
    end
  end
end

function supervisor:loop()
  while true do
    self:poll()
    self:check_target_queue()
  end

end

function supervisor:handle_events(events, ctx)
  local event_queue=table.dup(events)
  local queue_start=1
  local event
  while start_queue<=#event_queue do
    event=event_stack[queue_start]
    queue_start=queue_start+1
    table.join(event_queue, self:run_plugins(event, ctx, "passive"))
    table.join(event_queue, self:run_plugins(event, ctx, "active"))
    if event.name=="new_page" and context.page.on_complete then
      table.join(event_queue, context:collect_events(context.page.request.on_complete(ctx,event)))
    end
  end
end

return supervisor

