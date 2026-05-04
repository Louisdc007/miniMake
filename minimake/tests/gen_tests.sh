#!/bin/bash
# Génère tous les fichiers de test
D="$(dirname "$0")"

# ── CLI FLAGS ──────────────────────────────────────────────
# Help output
cat > "$D/test_help.out_e" << 'EOF'
Usage: minimake [OPTIONS] [TARGETS]
    OPTIONS can be:
        "-h"            to display the helper
        "-p"            to print the rules and variables
        "-f" <filename> to change the make config file, "Makefile" by default
    TARGETS are target name of makefile rule to execute
EOF

# ── PARSING / -p MODE ──────────────────────────────────────
# Simple: no variables, one rule
printf 'all:\n\techo hello\n' > "$D/test_simple.in"
printf '# variables\n\n# rules\n(all):\n\t'"'"'echo hello'"'"'\n' > "$D/test_simple.out_p"
printf 'echo hello\nhello\n' > "$D/test_simple.out_e"

# Two variables
printf 'CC = gcc\nFLAGS = -Wall\n\nall:\n\techo done\n' > "$D/test_twovar.in"
printf '# variables\n'"'"'CC'"'"' = '"'"'gcc'"'"'\n'"'"'FLAGS'"'"' = '"'"'-Wall'"'"'\n\n# rules\n(all):\n\t'"'"'echo done'"'"'\n' > "$D/test_twovar.out_p"
printf 'echo done\ndone\n' > "$D/test_twovar.out_e"

# Variable referencing another
printf 'A = hello\nB = ${A} world\n\nall:\n\techo ${B}\n' > "$D/test_varref.in"
printf '# variables\n'"'"'A'"'"' = '"'"'hello'"'"'\n'"'"'B'"'"' = '"'"'hello world'"'"'\n\n# rules\n(all):\n\t'"'"'echo ${B}'"'"'\n' > "$D/test_varref.out_p"
printf 'echo hello world\nhello world\n' > "$D/test_varref.out_e"

# Variable defined after use (parse time → empty)
printf 'OBJ = ${SRC}.o\nSRC = main\n\nall:\n\techo ${OBJ}\n' > "$D/test_varafter.in"
printf '# variables\n'"'"'OBJ'"'"' = '"'"'.o'"'"'\n'"'"'SRC'"'"' = '"'"'main'"'"'\n\n# rules\n(all):\n\t'"'"'echo ${OBJ}'"'"'\n' > "$D/test_varafter.out_p"
printf 'echo .o\n.o\n' > "$D/test_varafter.out_e"

# Variable in recipe defined later (execution time → expanded)
printf 'all:\n\techo ${FOO}\n\nFOO = bar\n' > "$D/test_varexec.in"
printf 'echo bar\nbar\n' > "$D/test_varexec.out_e"
printf '# variables\n'"'"'FOO'"'"' = '"'"'bar'"'"'\n\n# rules\n(all):\n\t'"'"'echo ${FOO}'"'"'\n' > "$D/test_varexec.out_p"

# Undefined variable → empty
printf 'all:\n\techo a${NOPE}b\n' > "$D/test_undefvar.in"
printf 'echo ab\nab\n' > "$D/test_undefvar.out_e"

# Multiple recipes
printf 'all:\n\techo one\n\techo two\n\techo three\n' > "$D/test_multirecipe.in"
printf 'echo one\none\necho two\ntwo\necho three\nthree\n' > "$D/test_multirecipe.out_e"

# Rule with no deps
printf 'all:\n\techo no deps\n' > "$D/test_nodep.in"
printf '# variables\n\n# rules\n(all):\n\t'"'"'echo no deps'"'"'\n' > "$D/test_nodep.out_p"

# Rule with multiple deps
printf 'all: foo bar\n\techo all\n\nfoo:\n\techo foo\n\nbar:\n\techo bar\n' > "$D/test_multidep.in"
printf 'echo foo\nfoo\necho bar\nbar\necho all\nall\n' > "$D/test_multidep.out_e"
printf '# variables\n\n# rules\n(all): [foo] [bar]\n\t'"'"'echo all'"'"'\n(foo):\n\t'"'"'echo foo'"'"'\n(bar):\n\t'"'"'echo bar'"'"'\n' > "$D/test_multidep.out_p"

# Sparse rule (blank lines in recipe)
printf 'all:\n\techo first\n\n\techo second\n' > "$D/test_sparse.in"
printf 'echo first\nfirst\necho second\nsecond\n' > "$D/test_sparse.out_e"
printf '# variables\n\n# rules\n(all):\n\t'"'"'echo first'"'"'\n\t'"'"'echo second'"'"'\n' > "$D/test_sparse.out_p"

# Comment line + inline comment
printf '# comment\nVAR = test\n\n# another comment\nall:\n\techo ${VAR}\n\t# recipe comment\n' > "$D/test_comment.in"
printf 'echo test\ntest\n# recipe comment\n' > "$D/test_comment.out_e"

# Inline comment in variable
printf 'VAR = hello # world\n\nall:\n\techo ${VAR}\n' > "$D/test_inlinecomment.in"
printf 'echo hello \nhello\n' > "$D/test_inlinecomment.out_e"
printf '# variables\n'"'"'VAR'"'"' = '"'"'hello '"'"'\n\n# rules\n(all):\n\t'"'"'echo ${VAR}'"'"'\n' > "$D/test_inlinecomment.out_p"

# Variable in dependency (expanded at parse)
printf 'DEP = foo\n\nall: ${DEP}\n\techo all\n\nfoo:\n\techo foo\n' > "$D/test_varindep.in"
printf 'echo foo\nfoo\necho all\nall\n' > "$D/test_varindep.out_e"
printf '# variables\n'"'"'DEP'"'"' = '"'"'foo'"'"'\n\n# rules\n(all): [foo]\n\t'"'"'echo all'"'"'\n(foo):\n\t'"'"'echo foo'"'"'\n' > "$D/test_varindep.out_p"

# Variable in target
printf 'TGT = all\n\n${TGT}:\n\techo works\n' > "$D/test_varintarget.in"
printf 'echo works\nworks\n' > "$D/test_varintarget.out_e"
printf '# variables\n'"'"'TGT'"'"' = '"'"'all'"'"'\n\n# rules\n(all):\n\t'"'"'echo works'"'"'\n' > "$D/test_varintarget.out_p"

# Empty target after expansion → ignored
printf 'TGT = \n\n${TGT}:\n\techo should not appear\n\nreal:\n\techo real\n' > "$D/test_emptytarget.in"
printf '# variables\n'"'"'TGT'"'"' = '"'"''"'"'\n\n# rules\n(real):\n\t'"'"'echo real'"'"'\n' > "$D/test_emptytarget.out_p"

# Empty dep after expansion → ignored
printf 'NODEP = \n\nall: ${NODEP}\n\techo ok\n' > "$D/test_emptydep.in"
printf '# variables\n'"'"'NODEP'"'"' = '"'"''"'"'\n\n# rules\n(all):\n\t'"'"'echo ok'"'"'\n' > "$D/test_emptydep.out_p"
printf 'echo ok\nok\n' > "$D/test_emptydep.out_e"

# Rule without recipe → exit 0 silently
printf 'all:\n' > "$D/test_norule_recipe.in"
printf '' > "$D/test_norule_recipe.out_e"

# No dep rule -p
printf 'all:\n' > "$D/test_norecipe.in"
printf '# variables\n\n# rules\n(all):\n' > "$D/test_norecipe.out_p"

# ── TARGET SELECTION ────────────────────────────────────────
printf 'all:\n\techo all\n\nclean:\n\techo clean\n\nbuild:\n\techo build\n' > "$D/test_targets.in"
printf 'echo all\nall\n' > "$D/test_targets_default.out_e"
printf 'echo clean\nclean\n' > "$D/test_targets_clean.out_e"
printf 'echo build\nbuild\n' > "$D/test_targets_build.out_e"
printf 'echo all\nall\necho clean\nclean\n' > "$D/test_targets_two.out_e"

# ── DEPENDENCIES ───────────────────────────────────────────
# Simple dep
printf 'all: sub\n\techo all\n\nsub:\n\techo sub\n' > "$D/test_chain2.in"
printf 'echo sub\nsub\necho all\nall\n' > "$D/test_chain2.out_e"

# Three levels
printf 'a: b\n\techo a\n\nb: c\n\techo b\n\nc:\n\techo c\n' > "$D/test_chain3.in"
printf 'echo c\nc\necho b\nb\necho a\na\n' > "$D/test_chain3.out_e"

# Missing dep
printf 'all: missing\n\techo all\n' > "$D/test_missingdep.in"

# ── ERRORS ──────────────────────────────────────────────────
# Failed command
printf 'all:\n\tfalse\n' > "$D/test_fail.in"
printf 'false\nminimake: *** [all] Error 1\n' > "$D/test_fail.out_e"

# Fail mid recipe
printf 'all:\n\techo before\n\tfalse\n\techo after\n' > "$D/test_failmid.in"
printf 'echo before\nbefore\nfalse\nminimake: *** [all] Error 1\n' > "$D/test_failmid.out_e"

# Two targets, first fails, second runs
printf 'bad:\n\tfalse\n\ngood:\n\techo good\n' > "$D/test_continue.in"
printf 'false\nminimake: *** [bad] Error 1\necho good\ngood\n' > "$D/test_continue.out_e"

# Empty makefile
printf '' > "$D/test_empty.in"

# Unknown target
printf 'all:\n\techo all\n' > "$D/test_unknowntarget.in"

# notACommand → Error 127
printf 'all:\n\tnotACommand42\n' > "$D/test_notacmd.in"
printf 'notACommand42\nminimake: *** [all] Error 127\n' > "$D/test_notacmd.out_e"

# ── UP TO DATE ─────────────────────────────────────────────
printf 'utd_target: utd_dep\n\techo rebuilt\n' > "$D/test_uptodate.in"
printf 'minimake: '"'"'utd_target'"'"' is up to date.\n' > "$D/test_uptodate_fresh.out_e"
printf 'echo rebuilt\nrebuilt\n' > "$D/test_uptodate_stale.out_e"

# Up to date no deps
printf 'myfile:\n\techo rebuilt\n' > "$D/test_uptodate_nodep.in"

# ── MULTIPLE -f (last wins) ────────────────────────────────
printf 'all:\n\techo second\n' > "$D/test_secondf.in"

# ── VAR trailing spaces ──────────────────────────────────
printf 'FOO =   42   \n\nall:\n\techo ${FOO}\n' > "$D/test_trailspace.in"
printf 'echo 42   \n42\n' > "$D/test_trailspace.out_e"
printf '# variables\n'"'"'FOO'"'"' = '"'"'42   '"'"'\n\n# rules\n(all):\n\t'"'"'echo ${FOO}'"'"'\n' > "$D/test_trailspace.out_p"

echo "All test files generated."
