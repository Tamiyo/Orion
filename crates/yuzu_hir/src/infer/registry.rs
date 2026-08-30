use yuzu_types::{AggFunc, BuiltinFunc};

/// A function guaranteed by the target dialect. Validation reads the metadata
/// here — the builtin's kind and arity — rather than matching on the function
/// itself, so new entries and kinds extend the language without touching the
/// checks.
pub(crate) struct Entry {
    pub(crate) func: BuiltinFunc,
    pub(crate) min_args: usize,
    pub(crate) max_args: usize,
}

pub(crate) const ENTRIES: &[Entry] = &[
    Entry {
        func: BuiltinFunc::Aggregate(AggFunc::Count),
        min_args: 0,
        max_args: 1,
    },
    Entry {
        func: BuiltinFunc::Aggregate(AggFunc::CountDistinct),
        min_args: 1,
        max_args: 1,
    },
    Entry {
        func: BuiltinFunc::Aggregate(AggFunc::Sum),
        min_args: 1,
        max_args: 1,
    },
    Entry {
        func: BuiltinFunc::Aggregate(AggFunc::Min),
        min_args: 1,
        max_args: 1,
    },
    Entry {
        func: BuiltinFunc::Aggregate(AggFunc::Max),
        min_args: 1,
        max_args: 1,
    },
    Entry {
        func: BuiltinFunc::Aggregate(AggFunc::Avg),
        min_args: 1,
        max_args: 1,
    },
];

pub(crate) fn entry(func: BuiltinFunc) -> &'static Entry {
    ENTRIES
        .iter()
        .find(|entry| entry.func == func)
        .expect("every builtin is registered")
}
