# Zeppelin

## Setup
This repo is setup as a T2 "star topology" meaning the apps are in the same repository as the manifest file. This means you should not clone this repo directly; instead, you should use `west` to clone it into a "west workspace" directory.

1. Install System Package Dependencies
- [uv](https://docs.astral.sh/uv/getting-started/installation/)
- [Zephyr System Package Dependencies](https://docs.zephyrproject.org/latest/develop/getting_started/index.html#install-dependencies)
2. Clone this repo
```bash
uvx west init -m https://github.com/jhammerberg/zeppelin.git --mr main zeppelin-workspace
```
3. Make west venv
```bash
cd zeppelin-workspace/zeppelin
uv venv --python 3.12
<activate venv for your shell>
uv pip install pip west
```
4. Install Zephyr Dependencies
```bash
west update
```
> [!NOTE]
> This may take a while and use quite a bit of space!
5. Install all west packages
```bash
west packages uv --install
```
> [!NOTE]
> This is an override of the native `west packages` command to support `uv`, you can find the implementation in `scripts/west_commands/packages_uv.py`
6. Install the Zephyr SDK
```bash
west sdk install
west zephyr-export
```
7. Open your code editor and build any project like this:
```bash
west build akron-app
```
> [!NOTE]
> You will need to build at least once before your editor will pick up on the libraries and intellisense.
