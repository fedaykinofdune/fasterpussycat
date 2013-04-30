shoggoth.connect_endpoint("tcp://127.0.0.1:9999")
local supervisor=require("fasterpussycat.core.supervisor")
supervisor:initialize()
supervisor:new_target("www.circlesoft.net")

