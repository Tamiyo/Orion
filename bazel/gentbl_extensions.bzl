"""Module extensions for MLIR-TableGen."""

load("@llvm-project//mlir:tblgen.bzl", "gentbl_filegroup")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

def gentbl_cc_library(
        name,
        tblgen,
        td_file,
        tbl_outs,
        td_srcs = [],
        includes = [],
        deps = [],
        strip_include_prefix = None,
        include_prefix = None,
        test = False,
        copts = None,
        **kwargs):
    filegroup_name = name + "_filegroup"

    gentbl_filegroup(
        name = filegroup_name,
        tblgen = tblgen,
        td_file = td_file,
        tbl_outs = tbl_outs,
        td_srcs = td_srcs,
        includes = includes,
        deps = deps,
        test = test,
        skip_opts = ["-gen-op-doc"],
        **kwargs
    )

    cc_library(
        name = name,
        # strip_include_prefix does not apply to textual_hdrs.
        # https://github.com/bazelbuild/bazel/issues/12424
        hdrs = [":" + filegroup_name] if strip_include_prefix else [],
        strip_include_prefix = strip_include_prefix,
        include_prefix = include_prefix,
        textual_hdrs = [":" + filegroup_name],
        copts = copts,
        **kwargs
    )
