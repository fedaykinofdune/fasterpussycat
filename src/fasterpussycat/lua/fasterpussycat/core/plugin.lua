local plugin={}
plugin.__index=plugin

plugin.PASSIVE=0
plugin.BROWSER=1
plugin.CRAWLER=2
plugin.HUNTER=3
plugin.EXPLOIT=4

plugin.QUICK=1
plugin.NORMAL=2
plugin.INTENSE=4
plugin.ALL_LEVELS=7

function plugin.new()
  p={}
  p.triggers={}
  p.triggers.new_target={}
  p.triggers.new_page={}
  p.triggers.target_keys={}
  p.triggers.page_keys={}
  setmetatable(p,plugin)
  return p
end


function plugin:on_new_target(func)
  self.triggers.new_target=func
end

function plugin:on_new_page(func)
  self.triggers.new_page=func
end


function plugin:on_target_key_change(key, func)
  if !p.triggers.target_keys[key] then
    p.triggers.target_keys[key]={}
  end
  table.insert(p.triggers.target_keys[key],func)
end

  
  
function plugin:on_target_key_equals(key, value, func)
  self:on_target_key_change(key, function(ctx, k, v)
    if(v==value) then
      func(ctx)
    end
  end
  )
end

function plugin:on_page_key_change(key, func)
  if !p.triggers.page_keys[key] then
    p.triggers.page_keys[key]={}
  end
  table.insert(p.triggers.page_keys[key],func)
end

function plugin:on_header_matches(header, match, func)
  self:on_new_page(function (ctx)
    if ctx.page.header[header] and ctx.page.header[header].find(match) then
      func(ctx)
    end
 end)

end

function plugin.tag_target(key, value)
  return function(ctx)
    ctx.target[key]=value
  end
end


function plugin.tag_page(key, value)
  return function(ctx)
    ctx.page[key]=value
  end
end


function plugin:on_page_key_equals(key, value, func)
  self:on_page_key_change(key, function(ctx, k, v)
    if(v==value) then
      func(ctx)
    end
  end
  )
end

plugin.basic_auth_bruteforce=require('fasterpussycat.core.plugin_helpers.basic_auth_bruteforce')
plugin.test_urls=require('fasterpussycat.core.plugin_helpers.test_urls')
return plugin
