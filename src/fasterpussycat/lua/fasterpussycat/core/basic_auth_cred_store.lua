local basic_auth_cred_store={}

function basic_auth_cred_store.new()
  t={}
  setmetatable(t,basic_auth_cred_store)
  t.credentials={}
  t.by_realm={}
  return t
end

function basic_auth_cred_store:add_credentials(creds)
  local c={path=shoggoth.dirname(creds.path),username=creds.username,password=creds.password,creds.realm=realm}
  table.insert(self.credentials,c)
  t.by_realm[realm]=c
end

function basic_auth_cred_store:find_credentials_by_path(path)
  local longest=""
  local ret=nil
  local s
  for _,cred in ipairs(self.credentials) do
    s,_=path:match(cred.path,1,true)
    if s==1 and #cred.path > #longest then
      longest=cred.path
      ret=cred
    end
  end
  return ret
end

return basic_auth_cred_store



