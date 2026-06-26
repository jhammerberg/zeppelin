# Zeppelin

## Setup
This repo is setup as a T2 "star topology" meaning the apps are in the same repository as the manifest file. Zephyr project layouts are confusing, so this means you have to clone this repository inside of a larger "west workspace" directory where west will actually clone the zephyr-rtos and other modules/dependencies into because "west workspaces" should not be Git repositories according to the Zephyr docs.

Also, Zephyr has like a bajillion Python dependencies, so to make the version control process easier this repository is also technically a uv project, just without any Python files so you can use uv sync to setup the venv and dependencies. While this is a bit silly, if you do want to make some Python scripts in the future for whatever, you'll already have a uv project setup!

1. Install System Package Dependencies
- [uv](https://docs.astral.sh/uv/getting-started/installation/)
- [Zephyr System Package Dependencies](https://docs.zephyrproject.org/latest/develop/getting_started/index.html#install-dependencies)
1. Clone this repo
```bash
mkdir -p <projects-folder>/zeppelin-workspace/zeppelin
git clone <this-url> <projects-folder>/zeppelin-workspace/zeppelin
cd <projects-folder>/zeppelin-workspace/zeppelin
```
2. Initialize the venv to get west
```bash
uv sync
<activate venv for your os>
```
3. Clone the Zephyr RTOS
```bash
west init -l .
west update
```
> This will use about 7GB and take a long time!
4. Install the Zephyr SDK
```bash
west sdk install
west zephyr-export
```
> If you get python errors, try running `uv add -r ../zephyr/scripts/requirements.txt`
