local p=plugin.new
p.aggressiveness=plugin.BROWSER
p.on_new_target(function(ctx)
      ctx.get("/")
      end)
return p

