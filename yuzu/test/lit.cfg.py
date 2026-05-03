# Lit configuration for the yuzu-tblgen golden tests.
#
# Each test is a TableGen `.td` file with `// RUN: ...` and `// CHECK: ...`
# directives at the top. Substitutions:
#
#   %yuzu-tblgen   path to the freshly-built yuzu-tblgen binary
#   %FileCheck     path to FileCheck
#   %not           inverts the exit code (negative tests)
#   %S             directory containing the test file
#   %td-include    `-I` path passed to yuzu-tblgen (yuzu/include)

import os

import lit.formats

config.name = "yuzu-tblgen"
config.test_format = lit.formats.ShTest(execute_external=True)
config.suffixes = [".td"]

config.test_source_root = os.path.dirname(__file__)
config.test_exec_root = os.path.join(config.yuzu_build_dir, "yuzu", "test")

yuzu_tblgen = os.path.join(
    config.yuzu_build_dir, "yuzu", "tools", "tblgen", "yuzu-tblgen"
)
file_check = config.file_check
td_include = os.path.join(config.yuzu_source_dir, "yuzu", "include")

config.substitutions.append(("%yuzu-tblgen", yuzu_tblgen))
config.substitutions.append(("%FileCheck", file_check))
config.substitutions.append(("%not", "! "))
config.substitutions.append(("%td-include", td_include))
