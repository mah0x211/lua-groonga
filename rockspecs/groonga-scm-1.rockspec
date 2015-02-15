package = "groonga"
version = "scm-1"
source = {
    url = "git://github.com/mah0x211/lua-groonga.git"
}
description = {
    summary = "groonga bindings for lua",
    homepage = "https://github.com/mah0x211/lua-groonga",
    license = "MIT/X11",
    maintainer = "Masatoshi Teruya"
}
dependencies = {
    "lua >= 5.1"
}
external_dependencies = {
    GROONGA = {
        header = "groonga/groonga.h",
        library = "groonga"
    }
}
build = {
    type = "builtin",
    modules = {
        groonga = {
            sources = {
                "src/lgroonga.c",
                "src/table.c"
            },
            libraries = { "groonga" },
            incdirs = {
                "$(GROONGA_INCDIR)"
            },
            libdirs = {
                "$(GROONGA_LIBDIR)"
            }
        }
    }
}


