local p=Plugin.new
p.aggressiveness=p.PASSIVE
p:on_header_matches("server","[Aa]pache", Plugin.tag_target("server", "apache"))
