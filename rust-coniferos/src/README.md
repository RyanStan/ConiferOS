Maybe I don't need cargo... I can just use rustc
[profile.dev]
panic = "abort"

[profile.release]
panic = "abort"


https://www.perplexity.ai/search/what-is-the-output-file-of-bui-vOi.yEyYSjiCtDb1WBpSXQ

https://doc.rust-lang.org/std/keyword.extern.html

Compile a main executable and link it to your lib. Use extern C to make things callable between files.
`rustc main.rs -L. -lmylib`. Rustc probably compiles main.rs to have the correct entry point.
