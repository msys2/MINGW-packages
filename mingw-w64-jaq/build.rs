fn main() {
  println!("cargo:rustc-link-search=native=@MINGW_PREFIX@/lib");
  println!("cargo:rustc-link-lib=dylib=mimalloc");
}
