import subprocess

Import("env")

def get_git_version():
    """Get version from latest git tag, fallback to 'dev' if no tags exist."""
    try:
        # Get the latest tag
        result = subprocess.run(
            ["git", "describe", "--tags", "--abbrev=0"],
            capture_output=True,
            text=True,
            check=True
        )
        return result.stdout.strip()
    except subprocess.CalledProcessError:
        # No tags found, try to get commit hash
        try:
            result = subprocess.run(
                ["git", "rev-parse", "--short", "HEAD"],
                capture_output=True,
                text=True,
                check=True
            )
            return "dev-" + result.stdout.strip()
        except subprocess.CalledProcessError:
            return "dev"

# Get version from git
git_version = get_git_version()

# Add the version to build flags
env.Append(CPPDEFINES=[
    ("GIT_VERSION", env.StringifyMacro(git_version)),
    ("BUILD_VERSION", env.StringifyMacro(git_version))
])

my_flags = env.ParseFlags(env["BUILD_FLAGS"])
defines = dict()
for b in my_flags.get("CPPDEFINES"):
    if isinstance(b, list):
        defines[b[0]] = b[1]
    else:
        defines[b] = b

# Use git version for the binary name
if defines.get("DEBUG") == "1":
    env.Replace(
        PROGNAME="%s_%s_%s"
        % (str(defines.get("BINARY_NAME")), git_version, "debug")
    )
else:
    env.Replace(PROGNAME="%s_%s" % (defines.get("BINARY_NAME"), git_version))

print(f"Building with version: {git_version}")
