local p=Plugin.new()
p.aggressiveness=p.HUNTER

local urls={
  {"/.git/index", Plugin.tag_page("git-found", true)},
}
p:on_new_target(Plugin.test_urls(urls))

return p
