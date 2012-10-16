faster.helpers={}

faster.helpers.normalize_url=function(url) 
  local ret=url
  if not url.find("^https?://") then ret="http://" .. url end
  if not url.find("/") then ret=ret .. "/" end
  return ret
  end


faster.helpers.rand_string=function(length)
  local ret=""
  local chars="abcdefghijklmnopqrstuvwxyz"
  local i
  for i = 1, length, 1 do
    ret=ret+chars[math.random(string.len(chars))]
  end
  return ret
  end



