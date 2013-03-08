function pt(table, indent)
    i=" "
    if(indent==nil) then
      indent=0
    end
    for k,v in pairs(table) do
      if(type(v)=="table") then
        io.write(i:rep(indent))
        print(k.." ==>")
        pt(v,indent+4)
    else
        io.write(i:rep(indent))
        print(k.." = '"..v.."'")
      end
    end
end


print("1")
shoggoth.connect_endpoint("tcp://127.0.0.1:9999")
print("2")
print("3")
local ret
local count=0
local i=0
print("enquing");
while i<60000 do

shoggoth.enqueue_http_request({port=80, path="/", host="localhost", method=0, headers={["Connection"]="keep-alive"}})
shoggoth.enqueue_http_request({port=80, path="/", host="localhost", method=0, headers={["Connection"]="keep-alive"}})

--   shoggoth.enqueue_http_request({port=443, path="/", host="localhost", method=0, options=1, headers={["Connection"]="keep-alive", ["Accept-Encoding"]="gzip"}})
  
--   shoggoth.enqueue_http_request({port=443, path="/", host="localhost", method=0, options=1, headers={["Connection"]="keep-alive", ["Accept-Encoding"]="gzip"}})
  i=i+1
end 
print("waiting...")
local start=os.time()
local now
local last=start

while true do 
  ret=shoggoth.poll()
  count=count+#ret
  now=os.time();
--  if #ret>1 then
--    pt(ret[1])
--    print()
--  end
  if (now-last>=2) then 
    print(count/(now-last))
    last=now
    count=0
  end
end
