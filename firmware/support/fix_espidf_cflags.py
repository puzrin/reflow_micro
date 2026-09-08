Import("env")

from SCons.Environment import Base


# Work around C-only flags leaking into C++ compilation in platform-espressif32.
# https://github.com/platformio/platform-espressif32/issues/1759 fixed only
# C++ flags leaking into C. Keep this suppression in CFLAGS instead of CCFLAGS.
def parse_flags(build_env, *flags):
    parsed = Base.ParseFlags(build_env, *flags)
    flag = "-Wno-old-style-declaration"
    if flag in parsed["CCFLAGS"]:
        parsed["CCFLAGS"].remove(flag)
        parsed["CFLAGS"].append(flag)
    return parsed


env.AddMethod(parse_flags, "ParseFlags")
