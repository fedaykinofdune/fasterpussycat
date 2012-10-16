
local old_reqindex=rawget(faster.http_request, "__index")
local old_resindex=rawget(faster.http_response,"__index")

rawset(faster.http_request, "__index",function(self, key)
    if key=="path" then return self:get_path() end
    if key=="path_only" then return self:get_path_only() end
    if key=="host" then return self:get_host() end
    if type(old_reqindex)=="function" then return old_reqindex(self,key) end
    return old_reqindex.__index(self,key)
  end)

rawset(faster.http_response, "__index",function(self, key)
    if key=="body" then return self:get_body() end
    if key=="code" then return self:get_code() end
    if type(old_resindex)=="function" then return old_resindex(self,key) end
    return old_resindex.__index(self,key)
  end)


faster.http_client.process_response_callback=function(req,res)
  local target=res.target
  res.target=target
  res.is_success=faster.is_page_success(res)
  req.do_callbacks(res)
  faster.do_callbacks(req,res)
  if target and res.is_success then target.blacklist_response(res) end
  end
