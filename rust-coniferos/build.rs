extern crate cbindgen;

use std::env;

fn main() {
    let crate_dir = env::var("CARGO_MANIFEST_DIR").unwrap();

    cbindgen::Builder::new()
        .with_crate(crate_dir)
        .with_language(cbindgen::Language::C)
        .with_no_includes()
        .with_sys_include("stdint.h")
        .with_sys_include("stdbool.h")
        .with_include_guard("RUST_CONIFEROS_H")
        .with_autogen_warning("// Auto-generated bindings for the rust-coniferos crate")
        .generate()
        .expect("Unable to generate bindings")
        .write_to_file("rust_coniferos.h");
}