use drop_bomb::DropBomb;

pub(crate) struct Marker {
    pub(crate) pos: usize,
    bomb: DropBomb,
}

impl Marker {
    pub(crate) fn new(pos: usize) -> Self {
        Self {
            pos,
            bomb: DropBomb::new("Markers must be completed!"),
        }
    }

    pub(crate) fn defuse(&mut self) {
        self.bomb.defuse();
    }
}

pub(crate) struct CompletedMarker {
    pub(crate) pos: usize,
}

impl CompletedMarker {
    pub(crate) fn new(pos: usize) -> Self {
        Self { pos }
    }
}
