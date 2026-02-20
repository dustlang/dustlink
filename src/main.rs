// Legacy placeholder.
// The active DustLink runtime entrypoint is now `src/main.ds`.
// Cargo is pinned to `src/main_legacy.rs` during transition.

fn main() {
    eprintln!("dustlink: use Dust entrypoint src/main.ds (legacy Rust shim still active)");
    std::process::exit(1);
}
