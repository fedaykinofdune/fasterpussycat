-- table utils

-- merge table src into dest

function table.merge(dest,src)
  for k,v in pairs(src) do
    dest[k]=v
  end
  return dest
end

function table.dup(t)
  return table.merge({},t)
end

-- join array src onto the end of array dest

function table.join(dest,src)
  for k,v in ipairs(src) do
    table.insert(dest,v)
  end
  return dest
end

return table
