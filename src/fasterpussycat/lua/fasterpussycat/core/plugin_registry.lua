local plugin_constants=require('fasterpussycat.core.plugin')
local plugin_registry={}
plugin_registry.ACTIVE=1
plugin_registry.PASSIVE=0
local ACTIVE=plugin_registry.ACTIVE
local PASSIVE=plugin_registry.PASSIVE
plugin_registry.plugins={}
plugin_registry.plugins.all={}
plugin_registry.plugins.enabled={}
plugin_registry.triggers={}

function plugin_registry:load_plugins(path)
  local handle=os.popen('find '..path..' -name "*.lua"') -- cheap and cheerful
  local name
  local plugin
  for line in handle:lines() do
    name=line:gsub(path,""):gsub(".lua","")
    print("loading plugin "..name)
    plugin=self:load_plugin(line)
    if plugin then
      plugin.name=name
      plugin.enabled=false
      self.plugins.all[name]=plugin
    end
  end
  
  handle:close()
  
end

function plugin_registry:find_runnable_plugins(event, cat)
  local t=triggers[event.name]
  if type(t)=="table" then
    if event.key and type(t[event.key)=="table" then
      t=t[event.key]
    end
    if t[cat] then
      return t[cat]
    end
  end
  return {}
end

function plugin_registry:can_enable_plugin(plugin)
  if type(plugin)=="string" then
    plugin=self.plugins.all[plugin]
  end
  if !plugin then
    return false
  end
  if plugin.force_disabled then
    return false
  else 
    for _,v in pairs(plugin.depends_on) do
      if !self:can_enable_plugin(v) then
        return false
      end
    end
  end
  return true
end

function plugin_registry:sort_plugins(table)
  if table[1] then
    table.sort(table, function(a,b)
      if !a.plugin or !b.plugin then
        return false
      end
      if a.plugin:is_passive()==b.plugin:is_passive() then
        if a.plugin:is_specific() then
          return true
        end
        return false
      end
      if a.plugin:is_passive() then
        return true
      end
      return false
    end)
  else
    for k,_ in pairs(table) do
      if type(k)=="table" then
        self:sort_plugins(table)
      end
    end
  end
end

function plugin_registry:harvest_triggers(plugin)
  local cat
  if plugin:is_passive() then
    cat="passive"
  else
    cat="active"
  end
  for k,v in pairs(plugin.triggers) do
    if !self.triggers[k][cat] then
      self.triggers[k][cat]={}
    end
    if type(plugin.triggers[k])=="function" then
      table.insert(self.triggers[k][cat], {plugin=plugin, func=plugin.triggers[k]})
    else
      for k1,v1 in pairs(plugin.triggers[k]) do
        if !self.triggers[k][k1][cat] then
          self.triggers[k][k1][cat]={}
        end
        table.insert(self.triggers[k][k1][cat], {plugin=plugin, func=v1})
      end
    end
  end
end

function plugin_registry:enable_plugin(plugin)

  if type(plugin)=="string" then
    plugin=self.plugins.all[plugin]
  end
  if !plugin then
    return false
  end


  if !self:can_enable_plugins(plugin) then
    return false
  end
  for _,v in pairs(plugin.dependencies) do
    if !self:enable_plugin(v) then
      return false
    end
  end
  
  plugin_registry.plugins.enabled[plugin.name]=plugin
  plugin_registry:harvest_triggers(plugin)
  return true
end

function plugin_registry:enable_plugins(opts)
  -- enable all plugins for now
  for _,v in pairs(self.plugins) do
    self:enable_plugin(v)
  end
  self:sort_plugins(self.triggers)
end

  
  


function plugin_registry:load_plugin(path)
  local status
  local ret
  status, ret=pcall(dofile, path)
  if !status then
    print(ret)
    return nil
  end
  return ret
end

return plugin_registry
