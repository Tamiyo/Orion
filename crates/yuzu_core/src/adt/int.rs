use std::cmp::Ordering;
use std::fmt;

#[derive(Copy, Clone, PartialEq, Eq, Hash, Debug)]
pub struct Int {
    raw: u64,
    num_bits: u32,
    signedness: Signedness,
}

impl Int {
    pub fn same_type(self, other: Self) -> bool {
        self.num_bits == other.num_bits && self.signedness == other.signedness
    }

    pub fn cast(self, num_bits: u32, signedness: Signedness) -> Self {
        let mask = if num_bits >= 64 {
            u64::MAX
        } else {
            (1u64 << num_bits) - 1
        };
        Self {
            raw: self.raw & mask,
            num_bits,
            signedness,
        }
    }

    pub fn checked_add(self, other: Self) -> Option<Self> {
        self.same_type(other)
            .then(|| self.value().checked_add(other.value()))
            .flatten()
            .and_then(|value| self.rewrap(value))
    }

    pub fn checked_sub(self, other: Self) -> Option<Self> {
        self.same_type(other)
            .then(|| self.value().checked_sub(other.value()))
            .flatten()
            .and_then(|value| self.rewrap(value))
    }

    pub fn checked_mul(self, other: Self) -> Option<Self> {
        self.same_type(other)
            .then(|| self.value().checked_mul(other.value()))
            .flatten()
            .and_then(|value| self.rewrap(value))
    }

    pub fn checked_div(self, other: Self) -> Option<Self> {
        if !self.same_type(other) || other.value() == 0 {
            return None;
        }
        self.value()
            .checked_div(other.value())
            .and_then(|value| self.rewrap(value))
    }

    pub fn checked_pow(self, other: Self) -> Option<Self> {
        if !self.same_type(other) || other.value() < 0 {
            return None;
        }
        let exponent = u32::try_from(other.value()).ok()?;
        self.value()
            .checked_pow(exponent)
            .and_then(|value| self.rewrap(value))
    }

    pub fn checked_shl(self, other: Self) -> Option<Self> {
        if !self.same_type(other) {
            return None;
        }
        let shift = other.value();
        if shift < 0 || shift >= self.num_bits as i128 {
            return None;
        }
        self.rewrap(self.value() << shift)
    }

    pub fn checked_shr(self, other: Self) -> Option<Self> {
        if !self.same_type(other) {
            return None;
        }
        let shift = other.value();
        if shift < 0 || shift >= self.num_bits as i128 {
            return None;
        }
        self.rewrap(self.value() >> shift)
    }

    pub fn checked_neg(self) -> Option<Self> {
        self.rewrap(-self.value())
    }

    pub fn compare(self, other: Self) -> Option<Ordering> {
        self.same_type(other)
            .then(|| self.value().cmp(&other.value()))
    }

    pub fn num_bits(self) -> u32 {
        self.num_bits
    }

    pub fn as_i64(self) -> i64 {
        self.value() as i64
    }

    fn value(self) -> i128 {
        match (self.num_bits, self.signedness) {
            (8, Signedness::Signed) => self.raw as i8 as i128,
            (16, Signedness::Signed) => self.raw as i16 as i128,
            (32, Signedness::Signed) => self.raw as i32 as i128,
            (64, Signedness::Signed) => self.raw as i64 as i128,
            _ => self.raw as i128,
        }
    }

    fn bounds(self) -> (i128, i128) {
        match (self.num_bits, self.signedness) {
            (8, Signedness::Signed) => (i8::MIN as i128, i8::MAX as i128),
            (16, Signedness::Signed) => (i16::MIN as i128, i16::MAX as i128),
            (32, Signedness::Signed) => (i32::MIN as i128, i32::MAX as i128),
            (64, Signedness::Signed) => (i64::MIN as i128, i64::MAX as i128),
            (8, Signedness::Unsigned) => (0, u8::MAX as i128),
            (16, Signedness::Unsigned) => (0, u16::MAX as i128),
            (32, Signedness::Unsigned) => (0, u32::MAX as i128),
            _ => (0, u64::MAX as i128),
        }
    }

    fn rewrap(self, value: i128) -> Option<Self> {
        let (min, max) = self.bounds();
        if value < min || value > max {
            return None;
        }
        let mask = if self.num_bits >= 64 {
            u64::MAX
        } else {
            (1u64 << self.num_bits) - 1
        };

        Some(Self {
            raw: (value as i64 as u64) & mask,
            num_bits: self.num_bits,
            signedness: self.signedness,
        })
    }
}

#[derive(Copy, Clone, PartialEq, Eq, Hash, Debug)]
pub enum Signedness {
    Signed,
    Unsigned,
}

impl From<i8> for Int {
    fn from(value: i8) -> Self {
        Self {
            raw: value as u8 as u64,
            num_bits: 8,
            signedness: Signedness::Signed,
        }
    }
}

impl From<i16> for Int {
    fn from(value: i16) -> Self {
        Self {
            raw: value as u16 as u64,
            num_bits: 16,
            signedness: Signedness::Signed,
        }
    }
}

impl From<i32> for Int {
    fn from(value: i32) -> Self {
        Self {
            raw: value as u32 as u64,
            num_bits: 32,
            signedness: Signedness::Signed,
        }
    }
}

impl From<i64> for Int {
    fn from(value: i64) -> Self {
        Self {
            raw: value as u64,
            num_bits: 64,
            signedness: Signedness::Signed,
        }
    }
}

impl From<u8> for Int {
    fn from(value: u8) -> Self {
        Self {
            raw: value as u64,
            num_bits: 8,
            signedness: Signedness::Unsigned,
        }
    }
}

impl From<u16> for Int {
    fn from(value: u16) -> Self {
        Self {
            raw: value as u64,
            num_bits: 16,
            signedness: Signedness::Unsigned,
        }
    }
}

impl From<u32> for Int {
    fn from(value: u32) -> Self {
        Self {
            raw: value as u64,
            num_bits: 32,
            signedness: Signedness::Unsigned,
        }
    }
}

impl From<u64> for Int {
    fn from(value: u64) -> Self {
        Self {
            raw: value,
            num_bits: 64,
            signedness: Signedness::Unsigned,
        }
    }
}

impl fmt::Display for Int {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match (self.num_bits, self.signedness) {
            (8, Signedness::Signed) => write!(f, "{}i8", self.raw as i8),
            (16, Signedness::Signed) => write!(f, "{}i16", self.raw as i16),
            (32, Signedness::Signed) => write!(f, "{}i32", self.raw as i32),
            (64, Signedness::Signed) => write!(f, "{}i64", self.raw as i64),
            (8, Signedness::Unsigned) => write!(f, "{}u8", self.raw as u8),
            (16, Signedness::Unsigned) => write!(f, "{}u16", self.raw as u16),
            (32, Signedness::Unsigned) => write!(f, "{}u32", self.raw as u32),
            (64, Signedness::Unsigned) => write!(f, "{}u64", self.raw),
            _ => write!(f, "{}?", self.raw),
        }
    }
}
