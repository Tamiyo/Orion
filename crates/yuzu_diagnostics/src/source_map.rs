#[derive(Clone, Copy, PartialEq, PartialOrd)]
pub struct SourceId(usize);

pub struct LineCol {
    pub line: usize,
    pub col: usize,
}

struct Entry {
    name: String,
    text: String,
    line_starts: Vec<usize>,
}

pub struct SourceMap {
    entries: Vec<Entry>,
}

impl Default for SourceMap {
    fn default() -> Self {
        Self::new()
    }
}

impl SourceMap {
    pub fn new() -> Self {
        Self {
            entries: Vec::new(),
        }
    }

    pub fn add(&mut self, name: String, text: String) -> SourceId {
        let line_starts = index_lines(&text);
        self.entries.push(Entry {
            name,
            text,
            line_starts,
        });
        SourceId(self.entries.len())
    }

    pub fn name(&self, source_id: SourceId) -> &str {
        &self.entries[source_id.0 - 1].name
    }

    pub fn text(&self, source_id: SourceId) -> &str {
        &self.entries[source_id.0 - 1].text
    }

    pub fn line_col(&self, source_id: SourceId, offset: usize) -> LineCol {
        let entry = &self.entries[source_id.0 - 1];
        let line_index = entry.line_starts.partition_point(|&start| start <= offset) - 1;

        LineCol {
            line: line_index + 1,
            col: offset - entry.line_starts[line_index] + 1,
        }
    }

    pub fn line_text(&self, source_id: SourceId, line: usize) -> &str {
        let entry = &self.entries[source_id.0 - 1];

        if line == 0 || line > entry.line_starts.len() {
            return "";
        }

        let start = entry.line_starts[line - 1];
        let end = entry
            .line_starts
            .get(line)
            .copied()
            .unwrap_or(entry.text.len());

        let text = &entry.text[start..end];
        let text = text.strip_suffix('\n').unwrap_or(text);
        text.strip_suffix('\r').unwrap_or(text)
    }
}

fn index_lines(text: &str) -> Vec<usize> {
    let mut starts = vec![0];
    starts.extend(text.match_indices('\n').map(|(i, _)| i + 1));
    starts
}
