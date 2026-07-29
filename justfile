# Zeppelin project automation
# Mostly exists to not have to activate the python venv by wrapping `uv run`

# Generic west wrapper
west *ARGS:
    uv run west {{ARGS}}

# First time setup
setup:
    @echo "{{YELLOW}}{{BOLD}}Running first time setup...{{NORMAL}}"
    # NOTE: Python 3.13+ will break setup because of changes to filesystem paths
    uv venv --python 3.12 --clear
    uv pip install pip west
    just update

# Auto-update
update:
    @echo "{{BLUE}}Checking for updates...{{NORMAL}}"
    git fetch
    uv run west update
    uv run west packages-uv uv --install
    uv run west sdk install
    uv run west zephyr-export
    uv run west blobs fetch hal_espressif

flash:
    uv run west flash

# TODO: make into generic serial monitor with something like minicom
default_baud := "115200"
console baud=default_baud:
    @echo "{{BLUE}}Starting serial console with baud rate: {{GREEN}}{{baud}}{{NORMAL}}"
    uv run west espressif monitor

[arg("sim", long, value="true")]
build target sim="false":
    @echo "{{BLUE}}Building{{NORMAL}} {{target}}..."
    uv run west build {{target}} -p auto {{ if sim == "true" { "-b native_sim" } else { "" } }}
