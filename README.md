# Custom Car (BakkesMod plugin)
Enables custom cars in Rocket League, for free!

## ✨ Features
- Use custom 3D models for car bodies and toppers
- Spawn and use any existing RL car body
    - Perfect for test-driving cars before making a purchase in the RL item shop 👌

## 🧐 How to make custom cars
Here is a video tutorial: https://youtu.be/OlwnVdYyhbk
  - Just put the JSON files in `bakkesmod\data\CustomCar\CustomCars` instead of the `acplugin` folder shown in the video

## 💻 Installation
Find the [latest release](https://github.com/smallest-cock/CustomCar/releases/latest) and follow the install steps ✅

## 🔨 Building
> [!NOTE]  
> Building requires **64-bit Windows** and the **MSVC** toolchain, due to reliance on the Windows SDK and the need for ABI compatibility with Rocket League

### 1. Initialize submodules
Run `./scripts/init-submodules.bat` to initialize the submodules after cloning the repo

<details> <summary>🔍 Why this instead of <code>git submodule update --init --recursive</code> ?</summary>
   <ul>
       <li>Avoids downloading 200MB of history for the <strong>nlohmann/json</strong> library</li>
       <li>Ensures Git can detect updates for the other submodules</li>
   </ul>
</details>

### 2. Build with CMake
> [!NOTE]
> Before building with CMake, the MSVC environment **must** be initialized.
> This is normally handled automatically by IDEs or certain editor extensions like [CMake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools), but if you're building from the command line, use one of the following methods:
>
> - Use an appropriate Windows terminal profile:
>    - `Developer PowerShell for VS 2022`
>    - `Developer Command Prompt for VS 2022`
> - Or run this script once per shell session:
>   ```
>   C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat
>   ```

0. Install [CMake](https://cmake.org/download) and [Ninja](https://github.com/ninja-build/ninja/releases)
   - If you prefer another build system, just create a `CMakeUserPresets.json` and specify it there. [More info here](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html)
1. Run this to configure build files:
    ```
    cmake --preset ninja-release
    ```
   - Other configure presets are available in `CMakePresets.json`
2. Run this to build:
    ```
    cmake --build --preset Ninja-Release
    ```
   - Other build presets are available in `CMakePresets.json`
   - The built binaries will be in `./plugins`