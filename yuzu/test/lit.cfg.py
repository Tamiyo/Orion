# pyright: reportUndefinedVariable=false
# `config` and `lit_config` are injected as globals by lit when it exec()s this
# file, so they're never imported here — silence the false "undefined" warning.

# Lit configuration for the yuzu golden tests.
#
# Two kinds of test, each carrying its own `// RUN: ...` and `// CHECK: ...`
# directives (both languages use `//` line comments, so the tools ignore them):
#
#   tools/tblgen/**/*.td   TableGen golden tests, run through yuzu-tblgen.
#   yuzu/**/*.yz           Yuzu programs, run through yuzu-compile (FileCheck
#                          its diagnostics and `--debug-*` dumps).
#
# Substitutions:
#
#   %yuzu-tblgen    the freshly-built yuzu-tblgen binary
#   %yuzu-compile   the freshly-built yuzu-compile binary
#   %FileCheck      FileCheck
#   %not            inverts the exit code (negative tests)
#   %s / %S         the test file / its directory
#   %td-include     `-I` path passed to yuzu-tblgen (yuzu/include)

import os

import lit.formats

config.name = "yuzu"
config.test_format = lit.formats.ShTest(execute_external=True)
config.suffixes = [".td", ".yz"]

config.test_source_root = os.path.dirname(__file__)
config.test_exec_root = os.path.join(config.yuzu_build_dir, "yuzu", "test")

tools = os.path.join(config.yuzu_build_dir, "yuzu", "tools")
yuzu_tblgen = os.path.join(tools, "tblgen", "yuzu-tblgen")
yuzu_compile = os.path.join(tools, "compile", "yuzu-compile")
file_check = config.file_check
td_include = os.path.join(config.yuzu_source_dir, "yuzu", "include")

config.substitutions.append(("%yuzu-tblgen", yuzu_tblgen))
config.substitutions.append(("%yuzu-compile", yuzu_compile))
config.substitutions.append(("%FileCheck", file_check))
config.substitutions.append(("%not", "! "))
config.substitutions.append(("%td-include", td_include))
