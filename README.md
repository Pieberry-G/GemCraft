# GemCraft
***

## Getting Started
Visual Studio 2019 or 2022 is recommended, GemCraft is officially untested on other development environments whilst we focus on a Windows build.

<ins>**1. Downloading the repository:**</ins>

Start by cloning the repository with `git clone --recursive https://github.com/Pieberry-G/GemCraft.git`.

If the repository was cloned non-recursively previously, use `git submodule update --init` to clone the necessary submodules.

<ins>**2. Configuring the dependencies:**</ins>

1. Install CMake.
2. Run the [GenerateProjects.bat](https://github.com/Pieberry-G/GemCraft/blob/main/GenerateProjects.bat) file. This will use administrator privileges to run CMake and generate a Visual Studio solution file in the build folder for user's usage.
3. It is recommended to compile in Release mode to achieve higher runtime efficiency.