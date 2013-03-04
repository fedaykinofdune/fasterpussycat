print("1")
shoggoth.connect_endpoint("tcp://127.0.0.1:9999")
print("2")
shoggoth.enqueue_http_request({port=80, path="/", host="www.google.com", method="GET", headers={}})
print("3")
local ret
while true do 
  ret=shoggoth.poll()
  if #ret>0 then
    print(ret[1].error)
    print(ret[1].body)
  end
end
