import os
import platform
import sys
from methods import get_compiler_version, using_gcc, using_clang


def is_active():
    return True


def get_name():
    return "Haiku"


def can_build():
    if (sys.platform.startswith('haiku')):
        return True

    return False


def get_opts():
    from SCons.Variables import BoolVariable, EnumVariable

    return [
        BoolVariable('use_lto', 'Use link time optimization (LTO)', False),
        BoolVariable('use_static_cpp', 'Link libgcc and libstdc++ statically for better portability', False),
        BoolVariable('use_ubsan', 'Use GCC compiler undefined behavior sanitizer (UBSAN)', False),
        BoolVariable('use_asan', 'Use GCC compiler address sanitizer (ASAN))', False),
        BoolVariable('use_lsan', 'Use GCC compiler leak sanitizer (LSAN))', False),
        BoolVariable('use_tsan', 'Use GCC compiler thread sanitizer (TSAN))', False),
        EnumVariable('debug_symbols', 'Add debugging symbols to release builds', 'yes', ('yes', 'no', 'full')),
        BoolVariable('separate_debug_symbols', 'Create a separate file containing debugging symbols', False),
    ]


def get_flags():

    return []


def configure(env):
    ## Build type

    if (env["target"] == "release"):
        if (env["optimize"] == "speed"): #optimize for speed (default)
            env.Prepend(CCFLAGS=['-O3'])
        else: #optimize for size
            env.Prepend(CCFLAGS=['-Os'])

        if (env["debug_symbols"] == "yes"):
            env.Prepend(CCFLAGS=['-g1'])
        if (env["debug_symbols"] == "full"):
            env.Prepend(CCFLAGS=['-g2'])

    elif (env["target"] == "release_debug"):
        if (env["optimize"] == "speed"): #optimize for speed (default)
            env.Prepend(CCFLAGS=['-O2'])
        else: #optimize for size
            env.Prepend(CCFLAGS=['-Os'])
        env.Prepend(CPPDEFINES=['DEBUG_ENABLED'])

        if (env["debug_symbols"] == "yes"):
            env.Prepend(CCFLAGS=['-g1'])
        if (env["debug_symbols"] == "full"):
            env.Prepend(CCFLAGS=['-g2'])

    elif (env["target"] == "debug"):
        env.Prepend(CCFLAGS=['-g3'])
        env.Prepend(CPPDEFINES=['DEBUG_ENABLED', 'DEBUG_MEMORY_ENABLED'])

    ## Architecture

    is64 = sys.maxsize > 2 ** 32
    if env["bits"] == "default":
        env["bits"] = "64" if is64 else "32"

    ## Compiler configuration

    if env['use_ubsan'] or env['use_asan'] or env['use_lsan'] or env['use_tsan']:
        env.extra_suffix += "s"

        if env['use_ubsan']:
            env.Append(CCFLAGS=['-fsanitize=undefined'])
            env.Append(LINKFLAGS=['-fsanitize=undefined'])

        if env['use_asan']:
            env.Append(CCFLAGS=['-fsanitize=address'])
            env.Append(LINKFLAGS=['-fsanitize=address'])

        if env['use_lsan']:
            env.Append(CCFLAGS=['-fsanitize=leak'])
            env.Append(LINKFLAGS=['-fsanitize=leak'])

        if env['use_tsan']:
            env.Append(CCFLAGS=['-fsanitize=thread'])
            env.Append(LINKFLAGS=['-fsanitize=thread'])

    if env['use_lto']:
        if env.GetOption("num_jobs") > 1:
            env.Append(CCFLAGS=['-flto'])
            env.Append(LINKFLAGS=['-flto=' + str(env.GetOption("num_jobs"))])
        else:
            env.Append(CCFLAGS=['-flto'])
            env.Append(LINKFLAGS=['-flto'])

    env.Append(CCFLAGS=['-pipe'])
    env.Append(LINKFLAGS=['-pipe'])

    env["CC"] = "gcc-x86"
    env["CXX"] = "g++-x86"

    ## Dependencies

    if not env["builtin_libwebp"]:
        env.ParseConfig("pkg-config libwebp --cflags --libs")

    # freetype depends on libpng and zlib, so bundling one of them while keeping others
    # as shared libraries leads to weird issues
    if env["builtin_freetype"] or env["builtin_libpng"] or env["builtin_zlib"]:
        env["builtin_freetype"] = True
        env["builtin_libpng"] = True
        env["builtin_zlib"] = True

    if not env["builtin_freetype"]:
        env.ParseConfig("pkg-config freetype2 --cflags --libs")

    if not env["builtin_libpng"]:
        env.ParseConfig("pkg-config libpng16 --cflags --libs")

    if not env['builtin_bullet']:
        # We need at least version 2.89
        import subprocess
        bullet_version = subprocess.check_output(['pkg-config', 'bullet', '--modversion']).strip()
        if str(bullet_version) < "2.89":
            # Abort as system bullet was requested but too old
            print("Bullet: System version {0} does not match minimal requirements ({1}). Aborting.".format(bullet_version, "2.89"))
            sys.exit(255)
        env.ParseConfig("pkg-config bullet --cflags --libs")

    if not env["builtin_enet"]:
        env.ParseConfig("pkg-config libenet --cflags --libs")

    if not env["builtin_squish"]:
        env.ParseConfig("pkg-config libsquish --cflags --libs")

    if not env["builtin_zstd"]:
        env.ParseConfig("pkg-config libzstd --cflags --libs")

    # Sound and video libraries
    # Keep the order as it triggers chained dependencies (ogg needed by others, etc.)

    if not env["builtin_libtheora"]:
        env["builtin_libogg"] = False  # Needed to link against system libtheora
        env["builtin_libvorbis"] = False  # Needed to link against system libtheora
        env.ParseConfig("pkg-config theora theoradec --cflags --libs")

    if not env["builtin_libvpx"]:
        env.ParseConfig("pkg-config vpx --cflags --libs")

    if not env["builtin_libvorbis"]:
        env["builtin_libogg"] = False  # Needed to link against system libvorbis
        env.ParseConfig("pkg-config vorbis vorbisfile --cflags --libs")

    if not env["builtin_opus"]:
        env["builtin_libogg"] = False  # Needed to link against system opus
        env.ParseConfig("pkg-config opus opusfile --cflags --libs")

    if not env["builtin_libogg"]:
        env.ParseConfig("pkg-config ogg --cflags --libs")

    if env["builtin_libtheora"]:
        list_of_x86 = ["x86_64", "x86", "i386", "i586"]
        if any(platform.machine() in s for s in list_of_x86):
            env["x86_libtheora_opt_gcc"] = True

    if not env["builtin_wslay"]:
        env.ParseConfig("pkg-config libwslay --cflags --libs")

    if not env["builtin_mbedtls"]:
        # mbedTLS does not provide a pkgconfig config yet. See https://github.com/ARMmbed/mbedtls/issues/228
        env.Append(LIBS=["mbedtls", "mbedcrypto", "mbedx509"])

    if not env["builtin_miniupnpc"]:
        # No pkgconfig file so far, hardcode default paths.
        env.Prepend(CPPPATH=["/system/develop/headers/miniupnpc"])
        env.Prepend(CPPPATH=["/system/develop/headers/x86/miniupnpc"])
        env.Append(LIBS=["miniupnpc"])

    # On Linux wchar_t should be 32-bits
    # 16-bit library shouldn't be required due to compiler optimisations
    if not env["builtin_pcre2"]:
        env.ParseConfig("pkg-config libpcre2-32 --cflags --libs")

    ## Flags

    env.Prepend(CPPPATH=['#platform/haiku'])
    env.Append(CPPDEFINES=['HAIKU_ENABLED', 'UNIX_ENABLED', 'OPENGL_ENABLED', 'GLES_ENABLED'])
    env.Append(CPPDEFINES=['MEDIA_KIT_ENABLED', 'MIDI2_KIT_ENABLED'])
    env.Append(LIBS=['be', 'game', 'media', 'midi2', 'network', 'bnetapi', 'z', 'GL'])
