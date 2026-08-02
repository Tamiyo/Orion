//! Runs the Python end-to-end suite against a freshly built wheel.
//!
//! Ignored by default so `cargo test` stays Rust-only and needs no venv; run it
//! with `cargo e2e`.

use std::path::{Path, PathBuf};
use std::process::Command;

#[test]
#[ignore = "needs the python venv; run with `cargo e2e`"]
fn python_end_to_end_suite() {
    let root = workspace_root();
    let venv = root.join(".venv");
    assert!(
        venv.join("bin/python").exists(),
        "no venv at {}\n\
         create one first:\n  \
         python3 -m venv .venv && .venv/bin/pip install maturin -r python/requirements.txt",
        venv.display(),
    );

    // Share cargo's target directory: maturin keeps its own by default, which
    // rebuilds pyo3 and the extension on every run instead of reusing them.
    run(
        Command::new(venv.join("bin/maturin"))
            .args(["develop", "-m", "crates/yuzu_python/Cargo.toml"])
            .args(["--target-dir", "target"])
            .current_dir(&root),
        "maturin develop",
    );

    run(
        Command::new(venv.join("bin/python"))
            .args(["-m", "pytest", "python/tests", "-q"])
            .current_dir(&root),
        "pytest",
    );
}

fn workspace_root() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR"))
        .ancestors()
        .nth(2)
        .expect("the crate sits two levels below the workspace root")
        .to_path_buf()
}

fn run(command: &mut Command, what: &str) {
    let status = command
        .status()
        .unwrap_or_else(|failure| panic!("could not start {what}: {failure}"));
    assert!(status.success(), "{what} failed with {status}");
}
