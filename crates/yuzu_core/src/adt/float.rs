use std::cmp::Ordering;
use std::fmt;

#[derive(Copy, Clone, PartialEq, Eq, Hash, Debug)]
pub struct Float {
    raw: u64,
    n_bits: u32,
}

impl Float {
    pub fn same_type(self, other: Self) -> bool {
        self.n_bits == other.n_bits
    }

    pub fn checked_add(self, other: Self) -> Option<Self> {
        self.same_type(other)
            .then(|| self.rewrap(self.value() + other.value()))
    }

    pub fn checked_sub(self, other: Self) -> Option<Self> {
        self.same_type(other)
            .then(|| self.rewrap(self.value() - other.value()))
    }

    pub fn checked_mul(self, other: Self) -> Option<Self> {
        self.same_type(other)
            .then(|| self.rewrap(self.value() * other.value()))
    }

    pub fn checked_div(self, other: Self) -> Option<Self> {
        self.same_type(other)
            .then(|| self.rewrap(self.value() / other.value()))
    }

    pub fn checked_pow(self, other: Self) -> Option<Self> {
        self.same_type(other)
            .then(|| self.rewrap(self.value().powf(other.value())))
    }

    pub fn checked_neg(self) -> Self {
        self.rewrap(-self.value())
    }

    pub fn compare(self, other: Self) -> Option<Ordering> {
        if !self.same_type(other) {
            return None;
        }
        self.value().partial_cmp(&other.value())
    }

    pub fn num_bits(self) -> u32 {
        self.n_bits
    }

    pub fn as_f64(self) -> f64 {
        self.value()
    }

    fn value(self) -> f64 {
        match self.n_bits {
            32 => f32::from_bits(self.raw as u32) as f64,
            _ => f64::from_bits(self.raw),
        }
    }

    fn rewrap(self, value: f64) -> Self {
        let raw = match self.n_bits {
            32 => (value as f32).to_bits() as u64,
            _ => value.to_bits(),
        };
        Self {
            raw,
            n_bits: self.n_bits,
        }
    }
}

impl From<f64> for Float {
    fn from(value: f64) -> Self {
        Self {
            raw: value.to_bits(),
            n_bits: 64,
        }
    }
}

impl From<f32> for Float {
    fn from(value: f32) -> Self {
        Self {
            raw: value.to_bits() as u64,
            n_bits: 32,
        }
    }
}

impl fmt::Display for Float {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self.n_bits {
            32 => write!(f, "{}f32", f32::from_bits(self.raw as u32)),
            _ => write!(f, "{}f64", f64::from_bits(self.raw)),
        }
    }
}
