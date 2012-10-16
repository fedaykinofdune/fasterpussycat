local probe={}
probe.process_probe = function(resp, data) resp.target.blacklist(resp) end
probe.launch_probes = function(target, data)
  local i
  for i = 1, 3, 1 do
    target.enqueue("/"+faster.helpers.rand_string(10),{"on_success"=>probe.process_probe, "method" = "GET"})
  end
  for i = 1, 3, 1 do
    target.enqueue("/"+faster.helpers.rand_string(10)+"/",{"on_success"=>probe.process_probe, "method" = "GET"})
  end
  for i = 1, 3, 1 do
    target.enqueue("/cgi-bin/"+faster.helpers.rand_string(10),{"on_success"=>probe.process_probe, "method" = "GET"})
  end
  for i = 1, 3, 1 do
    target.enqueue("/"+faster.helpers.rand_string(10)+".php",{"on_success"=>probe.process_probe, "method" = "GET"})
  end
  for i = 1, 3, 1 do
    target.enqueue("/"+faster.helpers.rand_string(10)+".html",{"on_success"=>probe.process_probe, "method" = "GET"})
  end
end


faster.register_plugin("probe", {
  "init" = function() faster.register_new_target_callback(probe.launch_probes, nil) end,
  "aggression" = faster.AGGRESSION_MEDIUM
})
