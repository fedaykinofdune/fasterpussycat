faster.target = {


  new = function(path)
    local t={}
    setmetatable(t, faster.target)
    t.prototype=faster.http_client.new_request(path)
    t.host=t.prototype.host
    t.path=t.prototype.path_only
    t.blacklist=faster.blacklist.new()
    return t;
  end,

  merge_path = function(self, path) return faster.helpers.merge_path(self, path) end,

  enqueue = function(self, path, opts)
      local p=self:merge_path(path) 
      local r=faster.http_client.new_request(p, opts)
      r.target=self
      faster.http_client.enqueue(r)
  end,

  blacklist_response = function(self, response)
    self.blacklist:blacklist_response(response)
  end,


  is_response_blacklisted = function(self, response)
    self.blacklist:is_response_blacklisted(response)
  end


}
