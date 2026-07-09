# Zeppelin

## Setup
This repo is setup as a T2 "star topology" meaning the apps are in the same repository as the manifest file. This means you should not clone this repo directly; instead, you should use `west` to clone it into a "west workspace" directory.

1. Install System Package Dependencies
- [uv](https://docs.astral.sh/uv/getting-started/installation/)
- [Zephyr System Package Dependencies](https://docs.zephyrproject.org/latest/develop/getting_started/index.html#install-dependencies)
2. Install `west`:
```bash
uv tool install west
```
3. Clone this repo
```bash
west init -m https://github.com/jhammerberg/zeppelin.git --mr main zeppelin-workspace
```
3. Clone the Zephyr RTOS
```bash
cd zeppelin-workspace
west update
```
> This will use about 7GB and take a long time!
4. Install the Zephyr SDK
```bash
uv tool install --with-requirements zephyr/scripts/requirements.txt west
west sdk install
west zephyr-export
```
5. Open your code editor and build any project like this:
```bash
cd zeppelin
west build akron-app
```
> Note that you will need to build at least once before your editor will pick up on the libraries and intellisense.
