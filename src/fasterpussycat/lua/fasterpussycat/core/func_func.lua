local func_func={}

function func_func.chain(new_function, old_function)
  local f=function(...)
    if type(new_function)=="function" then
      new_function(...)
    end
    if type(old_function)=="function" then
      old_function(...)
    end
  end
  return f
end

return func_func
