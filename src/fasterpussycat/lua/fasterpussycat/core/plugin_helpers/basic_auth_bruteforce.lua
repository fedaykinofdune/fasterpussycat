
function basic_auth_bruteforce(self, passwords, conditions)
  self:on_new_page(function(ctx)
    if ctx.page.needs_basic_auth() and (not conditions or conditions(ctx)) then
      local r=ctx.page.basic_auth_realm()
      local u
      local p
      for _,creds in passwords do
        u=creds[1]
        p=creds[2]
        ctx.get(ctx.request.path, {on_complete=function(ctx2)
          if ctx2.page.code~=401 and ctx.page.code~=403 then
            ctx2.basic_auth_credentials:add_credentials({path=ctx.request.path,username=u,password=p, realm=r})
            ctx2:throw_event({name="new_basic_auth_credentials", username=u, password=p, path=ctx.request.path})
          end
        end})
      end
    end
  end)
end

return basic_auth_bruteforce
