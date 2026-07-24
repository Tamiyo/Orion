fn main() -> pyo3_stub_gen::Result<()> {
    yuzu::stub_info()?.generate()?;
    Ok(())
}
