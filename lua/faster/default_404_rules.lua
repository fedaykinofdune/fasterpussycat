
-- rules are added into a heap, last rule added is the first that gets run

faster.default_404_rules.add_rule("code=403 and isdir", faster.RULE_SUCC)
faster.default_404_rules.add_rule("code in (200,500)", faster.RULE_SUCC)
faster.default_404_rules.add_rule("code in (200,500)", function(resp, data)
      if resp.target and resp.target.in_blacklist(resp) then return faster.RULE_FAIL end
      return faster.RULE_NEXT
    end);

-- if we receive over 15 directory responses in a row with a particular code, blacklist that code

faster.default_404_rules.add_rule('isdir', function(resp, data) 
    if !resp.target then return faster.RULE_NEXT end 
      if resp.target.last_dir_code==resp.code then
        resp.target.last_dir_count=resp.target.last_dir_count+1
      else
        resp.target.last_dir_code=resp.code
        resp.target.last_dir_count=resp.target.last_dir_count=1
      end
      if resp.target.last_dir_count>15 then
        resp.target.custom_rules.add_rule('code='+resp.code+' and isdir', faster.RULE_FAIL);
      end
      return faster.RULE_NEXT;
    end)



-- magic rules

faster.default_404_rules.add_rule('method in ("GET","POST") and path endswith ".tgz" and magicmime!="application/x-gzip"', faster.RULE_FAIL);
faster.default_404_rules.add_rule('method in ("GET","POST") and path endswith ".gz" and magicmime!="application/x-gzip"', faster.RULE_FAIL);
faster.default_404_rules.add_rule('method in ("GET","POST") and path endswith ".tar" and magicmime!="application/x-tar"', faster.RULE_FAIL);
faster.default_404_rules.add_rule('method in ("GET","POST") and path endswith ".zip" and magicmime!="application/zip"', faster.RULE_FAIL);
faster.default_404_rules.add_rule('method in ("GET","POST") and path endswith ".bz2" and magicmime!="application/x-bzip2"', faster.RULE_FAIL);
faster.default_404_rules.add_rule('method in ("GET","POST") and path endswith ".sql" and magicmime!="text/plain"', faster.RULE_FAIL);

-- quick fails

faster.default_404_rules.add_rule('size=0 and not isdir', faster.RULE_FAIL)
faster.default_404_rules.add_rule('code=404', faster.RULE_FAIL )


