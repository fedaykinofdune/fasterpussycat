local p=Plugin.new()
p.aggressiveness=p.HUNTER
p.depends_on={"analysis/detect_iis"}

local urls={
  {"/web.config.old", Plugin.tag_page("web.config-backup", true)},
  {"/web.config.bak", Plugin.tag_page("web.config-backup", true)},
  {"/web.config.OLD", Plugin.tag_page("web.config-backup", true)},
  {"/web.config.BAK", Plugin.tag_page("web.config-backup", true)},
  {"/web.config.backup", Plugin.tag_page("web.config-backup", true)},
  {"/web.config.new", Plugin.tag_page("web.config-backup", true)},
  {"/trace.axd", Plugin.tag_page("trace.axd", true)}

}
p:on_target_key_equals("server","iis", Plugin.test_urls(urls))

return p
