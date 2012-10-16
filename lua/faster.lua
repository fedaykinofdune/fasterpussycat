require('faster.helpers')
require('faster.target')
require('faster.http_client')

-- ******************
-- internal variables
-- ******************

faster.new_target_callbacks={}
faster.targets={}
faster.__target_str_to_target={}

-- *******
-- Methods
-- *******


-- is page request successful?

faster.is_page_success=function(resp)
  local rc
  if resp.target then
    rc=faster.target.custom_ruleset:run_rules(resp)
    if rc==faster.DETECT_FAIL then return faster.DETECT_FAIL end
  end
  rc=faster.default_ruleset:run_rules(resp)
  
  if rc==faster.DETECT_SUCCESS and resp.target and resp.target:is_response_blacklisted(resp) then return faster.DETECT_FAIL end
  return rc
  end

-- register a new target callback

faster.register_new_target_callback=function(func, data)
  faster.new_target_callbacks.insert({func,data})
  end

-- add target

faster.add_target=function(target_str)
  local normalized_target=faster.helpers.normalize_url(target_str)
  if faster.__target_str_to_targets[normalized_target] then return faster.__target_str_to_target[normalized_target] end
  local t=faster.target.new(normalized_target)
  faster.targets.insert(t)
  faster.__target_str_to_targets[target_str]=t
  for _, callback in faster.new_target_callbacks do
    callback[0](t, callback[1])
  end
  return t
  end

