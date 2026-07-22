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
uv venv
<activate venv for your shell>
uv pip install pip west
```
4. Install Zephyr Dependencies
```bash
west update
```
> This will use about 7GB and take a long time!
5. Install all west packages
```bash
west packages --install
```
If you're on a POSIX system, you can pipe this output to uv and have it install faster
```bash
west packages pip | xargs uv pip install
```
6. Install the Zephyr SDK
```bash
west sdk install
west zephyr-export
```
7. Open your code editor and build any project like this:
```bash
west build akron-app
```
> Note that you will need to build at least once before your editor will pick up on the libraries and intellisense.
