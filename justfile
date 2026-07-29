# Zeppelin project automation
# Mostly exists to not have to activate the python venv by wrapping `uv run`

[private] # private so it doesn't show itself in the list
default:
    @just --list

# Generic west wrapper
west *ARGS:
    uv run west {{ARGS}}

# uses clang-format from uv venv and finds files using git which should be guarenteed to exist
check:
    git ls-files "*.cpp" "*.h" "*.hpp" "*.cc" "*.c" | xargs uv run clang-format --dry-run --Werror -style=file || true

# clang-format all source files
format:
    git ls-files "*.cpp" "*.h" "*.hpp" "*.cc" "*.c" | xargs uv run clang-format -i -style=file || true

# First time setup
setup:
    @echo "{{YELLOW}}{{BOLD}}Running first time setup...{{NORMAL}}"
    # NOTE: Python 3.13+ will break setup because of changes to filesystem paths
    uv venv --python 3.12 --clear
    uv pip install pip west
    just update

# Auto-update west and Zephyr dependencies
update:
    @echo "{{BLUE}}Checking for updates...{{NORMAL}}"
    git fetch
    uv run west update
    uv run west packages-uv uv --install
    uv run west sdk install
    uv run west zephyr-export
    uv run west blobs fetch hal_espressif

# Flash the connected board
flash:
    uv run west flash

# TODO: make into generic serial monitor with something like minicom
default_baud := "115200"

# Open a serial console (default baud 115200)
console baud=default_baud:
    @echo "{{BLUE}}Starting serial console with baud rate: {{GREEN}}{{baud}}{{NORMAL}}"
    uv run west espressif monitor

# Build a target (pass --sim to build for native_sim)
[arg("sim", long, value="true")]
build target sim="false":
    @echo "{{BLUE}}Building{{NORMAL}} {{target}}..."
    uv run west build {{target}} -p auto {{ if sim == "true" { "-b native_sim" } else { "" } }}
