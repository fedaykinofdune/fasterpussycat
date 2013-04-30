local test_urls=function(urls)
  return function(ctx)
    for _,url in ipairs(urls) do
      local opts={on_exists=url[2]}
      ctx:get(url, opts)
    end
  end
end


return test_urls

