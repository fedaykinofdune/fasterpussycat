local p=Plugin.new()
p.aggressiveness=Plugin.BROWSER
p:on_new_target(function(ctx)
      ctx.get("/")
      end)
return p

