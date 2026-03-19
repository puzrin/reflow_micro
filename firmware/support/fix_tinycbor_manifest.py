Import("env")

from pathlib import Path


open_memstream_path = (
    Path(env.subst("$PROJECT_LIBDEPS_DIR"))
    / env["PIOENV"]
    / "tinycbor"
    / "src"
    / "open_memstream.c"
)

if open_memstream_path.exists():
    # Upstream TinyCBOR ships open_memstream.c for Unix-like hosts only.
    # On ESP it aborts the build with:
    #   #error "Cannot implement open_memstream!"
    # We don't use CBOR-to-JSON helpers, so drop that one non-portable file.
    open_memstream_path.unlink()
