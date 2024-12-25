Import("env")

def generate_compiledb(env, *args, **kwargs):
    env.Execute(f"pio run -e {env['PIOENV']} -t compiledb")

env.AddPreAction("buildprog", generate_compiledb)
