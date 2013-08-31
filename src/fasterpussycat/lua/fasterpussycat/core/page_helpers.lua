local page={}

function page:needs_basic_auth()
  return (self.code==401 and self.headers["WWW-Authenticate"] and self.headers["WWW-Authenticate"]:find("^[bB]asic"))
end

function page:exists()
  return (self.code==200)
end


function page:basic_auth_realm()
  if not self.headers["WWW-Authenticate"] then
    return nil
  end
  return self.headers["WWW-Authenticate"]:match("^[bB]asic (%S+)")
end

return page
