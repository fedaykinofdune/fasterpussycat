print("1")
shoggoth.connect_endpoint("tcp://127.0.0.1:9999")
print("2")
shoggoth.enqueue_http_request({port=80, path="/", host="www.google.com", method="get", headers={}})
print("3")
local ret
while true do 
  ret=shoggoth.poll()
  if #ret>0 then
    print(ret.body)
  end
end
