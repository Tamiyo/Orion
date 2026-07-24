use string_interner::{DefaultStringInterner, symbol::SymbolU32};

pub type SymbolId = SymbolU32;

#[derive(Default)]
pub struct StringInterner {
    strings: DefaultStringInterner,
}

impl StringInterner {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn intern(&mut self, text: &str) -> SymbolId {
        self.strings.get_or_intern(text)
    }

    pub fn text(&self, symbol: SymbolId) -> &str {
        self.strings
            .resolve(symbol)
            .expect("symbol should be interned")
    }
}
