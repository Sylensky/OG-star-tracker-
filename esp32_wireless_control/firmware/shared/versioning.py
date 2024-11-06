Import("env")
my_flags = env.ParseFlags(env["BUILD_FLAGS"])
defines = dict()
for b in my_flags.get("CPPDEFINES"):
    if isinstance(b, list):
        defines[b[0]] = b[1]
    else:
        defines[b] = b

if defines.get("DEBUG") == "1":
    env.Replace(
        PROGNAME="%s_%s_%s"
        % (str(defines.get("BINARY_NAME")), str(defines.get("VERSION")), "debug")
    )
else:
    env.Replace(PROGNAME="%s_%s" % (defines.get("BINARY_NAME"), defines.get("VERSION")))
