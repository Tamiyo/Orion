use yuzu_types::{AggFunc, BuiltinFunc};

/// A function guaranteed by the target dialect. Validation reads the metadata
/// here — the builtin's kind and arity — rather than matching on the function
/// itself, so new entries and kinds extend the language without touching the
/// checks.
pub(crate) struct Entry {
    pub(crate) func: BuiltinFunc,
    pub(crate) arity: usize,
}

pub(crate) const ENTRIES: &[Entry] = &[
    Entry {
        func: BuiltinFunc::Aggregate(AggFunc::Count),
        arity: 0,
    },
    Entry {
        func: BuiltinFunc::Aggregate(AggFunc::Sum),
        arity: 1,
    },
    Entry {
        func: BuiltinFunc::Aggregate(AggFunc::Min),
        arity: 1,
    },
    Entry {
        func: BuiltinFunc::Aggregate(AggFunc::Max),
        arity: 1,
    },
    Entry {
        func: BuiltinFunc::Aggregate(AggFunc::Avg),
        arity: 1,
    },
];

pub(crate) fn entry(func: BuiltinFunc) -> &'static Entry {
    ENTRIES
        .iter()
        .find(|entry| entry.func == func)
        .expect("every builtin is registered")
}
