PROT_HTTPS=1
PROT_HTTP=0

HttpRequest={}

HttpRequest.__index=HttpRequest

function HttpRequest.new(opts)
  hr={}
  setmetatable(hr,HttpRequest)
  if type(opts)=="string" then
  elseif type(opts)=="table" then
    for k,v in pairs(opts) do hr[k] = v end
  end
  return hr
end




