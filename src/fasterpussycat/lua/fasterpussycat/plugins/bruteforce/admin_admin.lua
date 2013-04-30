local p=plugin.new
p.aggressiveness=p.EXPLOIT
p.speed=p.QUICK

local passwords={
   {"admin","admin"},
   {"admin","password"},
   {"admin","default"},
   {"admin",""},
   {"root", "root"},
   {"root", "toor"}, 
   {"root", ""}
}
p:basic_auth_bruteforce(passwords)
           
return p
