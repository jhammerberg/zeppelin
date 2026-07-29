# Zeppelin project automation
# Mostly exists to not have to activate the python venv by wrapping `uv run`

setup:
    uv venv --python 3.12 --clear
    uv pip install pip west
    just update

update:
    git fetch
    uv run west update
    uv run west packages-uv uv --install
    uv run west sdk install
    uv run west zephyr-export
    uv run west blobs fetch hal_espressif

build target:
    @echo "{{BLUE}}Building{{NORMAL}} {{target}}..."
    uv run west build {{target}} -p auto

flash:
    uv run west flash

default_baud := "115200"
console baud=default_baud:
    @echo "{{BLUE}}Starting serial console with baud rate: {{GREEN}}{{baud}}{{NORMAL}}"
    @echo "{{RED}}{{BOLD}}Not yet implemented{{NORMAL}}"
