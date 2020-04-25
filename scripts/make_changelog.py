#!/usr/bin/python3
import re
import pygit2

repo = pygit2.Repository(".")

def rep(match) :
	commit_hash = match.group(0)
	commit,_ = repo.resolve_refish(commit_hash)
	return "[%s](https://github.com/horizon-eda/horizon/commit/%s)"%(commit.short_id, commit_hash)

with open("scripts/CHANGELOG.md.in", "r") as infile :
	changelog = infile.read()
	with open("CHANGELOG.md", "w") as outfile :
		outfile.write(re.sub(r"\b[0-9a-f]{40}\b", rep, changelog))
