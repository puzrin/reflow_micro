Import("env")

from pathlib import Path
from shutil import copyfile


tinycbor_path = (
    Path(env.subst("$PROJECT_LIBDEPS_DIR"))
    / env["PIOENV"]
    / "tinycbor"
)

# Upstream TinyCBOR ships open_memstream.c for Unix-like hosts only.
# On ESP it aborts the build with:
#   #error "Cannot implement open_memstream!"
# We don't use CBOR-to-JSON helpers, so drop that one non-portable file.
(tinycbor_path / "src" / "open_memstream.c").unlink(missing_ok=True)

# TinyCBOR 7 replaced ready-made headers with CMake templates. Copy them to
# the required .h names for PlatformIO; we don't use the version placeholders.
for name in ("tinycbor-version.h", "tinycbor-export.h"):
    copyfile(tinycbor_path / "src" / f"{name}.in", tinycbor_path / "src" / name)
