local p=Plugin.new
p.aggressiveness=p.HUNTER
p.depends_on={"analysis/detect_apache"}

local opts

local urls={
  {"/server-info", Plugin.tag_page("server-info", true)},
  {"/status", Plugin.tag_page("server-info", true)},
  {"/htaccess.old", Plugin.tag_page("htaccess-backup", true)},
  {"/htaccess.bak", Plugin.tag_page("htaccess-backup", true)},
  {"/logs/access.log", Plugin.tag_page("logfile", true)},
  {"/logs/access_log", Plugin.tag_page("logfile", true)},
  {"/logs/error_log", Plugin.tag_page("logfile", true)},
  {"/logs/error.log", Plugin.tag_page("logfile", true)},
}
p:on_target_key_equals("server","apache", Plugin.test_urls(urls))

return p
