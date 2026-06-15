#include "yuzu/Compiler/CompilePipeline.h"
#include "yuzu/Util/Unicode.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/string.h>

#include <string>

namespace nb = nanobind;

NB_MODULE(_ext, m) {
  // Compiler knobs. `out` (a raw_ostream reference) isn't exposed — Python
  // controls behaviour through the flags and the artifacts directory.
  nb::class_<yuzu::CompileOptions>(m, "CompileOptions")
      .def(nb::init<>())
      .def_rw("execute", &yuzu::CompileOptions::execute)
      .def_rw("debug_lexer", &yuzu::CompileOptions::debugLexer)
      .def_rw("debug_ast", &yuzu::CompileOptions::debugAst)
      .def_rw("debug_hir", &yuzu::CompileOptions::debugHir)
      .def_rw("debug_anf", &yuzu::CompileOptions::debugAnf)
      .def_rw("artifacts_dir", &yuzu::CompileOptions::artifactsDir);

  m.def(
      "compile",
      [](const std::string &source, yuzu::CompileOptions options) {
        yuzu::compile(yuzu::util::decodeUtf8(source), options);
      },
      nb::arg("source"), nb::arg("options") = yuzu::CompileOptions{},
      "Compile Yuzu source. With options.artifacts_dir set, writes the "
      "Substrait plan to <artifacts_dir>/plan.substrait.json.");
}
