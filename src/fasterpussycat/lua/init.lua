print("1")
shoggoth.connect_endpoint("tcp://127.0.0.1:9999")
print("2")
print("3")
local ret
local count=0
local i=0
while i<10000 do
  shoggoth.enqueue_http_request({port=80, path="/", host="localhost", method="GET", headers={["Connection"]="keep-alive"}})
  
  shoggoth.enqueue_http_request({port=80, path="/", host="localhost", method="GET", headers={["Connection"]="keep-alive"}})
  i=i+1
end 

local start=os.time()
local now
local last=start

while true do 
  ret=shoggoth.poll()
  count=count+#ret
  now=os.time();
  if #ret>1 then
    print(ret[1].status)
  end
  if (now-last>=10) then 
    last=now
    print(count/(now-start))
  end
end
