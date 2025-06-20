# [Angaraka]: A Modular 3D Game Engine

<div style="margin: 0 auto; height: 70vh; overflow:hidden; background:black; background: radial-gradient(ellipse at center, #1b0a23 0%, #000000 100%); display: flex; justify-content: center; align-items: center; background-repeat: no-repeat; background-attachment: fixed; background-size: cover;">
<img src="Angaraka.png" alt="Angaraka Game Engine" style="object-fit:cover; max-width:50%; height:auto;border-radius:30%">
</div>

---
Welcome to **Angaraka**, a modular and hot-reloadable 3D game engine being built from the ground up! This project aims to provide a flexible and powerful foundation for 3D game development on Windows, with a strong emphasis on a plugin-based architecture, first-class AI integration, and an in-engine editor.

---

## Table of Contents

* [Features](#features)
* [Architecture Overview](#architecture-overview)
* [Getting Started](#getting-started)
    * [Prerequisites](#prerequisites)
    * [Cloning the Repository](#cloning-the-repository)
    * [Project Setup (Visual Studio 2022)](#project-setup-visual-studio-2022)
    * [Building the Engine](#building-the-engine)
    * [Running the Application](#running-the-application)
* [Core Modules](#core-modules)
* [Hot Reloading](#hot-reloading)
* [AI Subsystem](#ai-subsystem)
* [The Editor](#the-editor)
* [Contributing](#contributing)
* [License](#license)
* [Contact](#contact)

---

## Features

* **Modular Architecture:** Designed with a plugin-based system where core subsystems (graphics, AI, physics, etc.) are loaded as dynamic-link libraries (DLLs).
* **Hot Reloading:** Experience rapid iteration with the ability to modify, recompile, and reload engine plugins at runtime without restarting the application.
* **First-Class AI:** AI is deeply integrated into the engine's core design, not an afterthought.
* **In-Engine Editor:** A requirement from day one, providing powerful tools for scene manipulation, content creation, and engine debugging.
* **Windows Platform Focus:** Built specifically for Windows using Visual Studio 2022 and leveraging platform-specific APIs where appropriate (e.g., DirectX 12 for rendering).
* **C++23 Powered:** Embracing modern C++ features for cleaner, more efficient, and robust code.

---

## Architecture Overview

Angaraka is structured around a **lightweight core** and **extensible plugins**.

* **`Engine.Core` (Static Library):** The stable foundation. It defines core interfaces (like `IPlugin`, `IGraphicsSystem`), provides fundamental utilities (math, logging), and, most importantly, hosts the `PluginManager`.
* **`Engine.Application` (Executable):** The main entry point for the game runtime. It initializes the `PluginManager`, loads necessary plugins, and manages the main game loop.
* **Plugins (Dynamic Link Libraries - DLLs):** Individual subsystems like `Engine.Graphics.DX12`, `Engine.AI`, `Engine.Physics`, etc. These implement the interfaces defined in `Engine.Core` and are hot-reloadable.
* **`Engine.Editor` (Executable):** A separate application that loads the same plugins as the game, providing a powerful graphical interface for content creators and engineers.

This architecture promotes **separation of concerns**, **loose coupling**, and **high extensibility**, making the engine adaptable and easy to develop collaboratively.

---

## Getting Started

Follow these steps to get the Angaraka up and running on your local machine.

### Prerequisites

* **Windows 10/11**
* **Visual Studio 2022:**
    * Ensure you have the following workloads installed:
        * `Desktop development with C++`
        * `Game development with C++`
    * Verify individual components like `Windows 10/11 SDK` (latest), `C++ profiling tools`, `C++ AddressSanitizer`, and `MSVC v143 build tools` (latest).
* **Git:** For cloning the repository.

### Cloning the Repository

Open your Git-enabled terminal (e.g., Git Bash, PowerShell, or Command Prompt) and run:

```bash
git clone https://github.com/Derthenier/Angaraka.git
cd Angaraka
```

### Project Setup (Visual Studio 2022)

1.  **Open Solution:** Navigate to the `Angaraka` directory and open the `Angaraka.sln` file in Visual Studio 2022.
2.  **Verify Project Structure:** In Solution Explorer, you should see:
    * `Engine.Application`
    * `Engine.Core`
    * (You will add `Engine.Graphics.DX12` and others later as per the roadmap)
3.  **Project Dependencies:** Ensure `Engine.Application` depends on `Engine.Core` for proper build order. (Right-click `Engine.Application` -> `Build Dependencies` -> `Project Dependencies...`).
4.  **Include Directories:** Confirm `Engine.Application`'s C++ properties include `$(SolutionDir)Engine.Core\` in `Additional Include Directories`.
5.  **DLL Project Settings (Crucial for Hot Reloading):**
    * For any project intended to be a hot-reloadable DLL (e.g., `Engine.Graphics.DX12` once created), ensure the following properties are set (for both Debug and Release configurations):
        * `C/C++` -> `General` -> `Debug Information Format`: `Program Database for Edit and Continue (/ZI)`
        * `Linker` -> `General` -> `Enable Incremental Linking`: `Yes (/INCREMENTAL)`
        * `C/C++` -> `Code Generation` -> `Runtime Library`: `Multi-threaded Debug DLL (/MDd)` (for Debug) / `Multi-threaded DLL (/MD)` (for Release)

### Building the Engine

1.  **Select Configuration:** Choose your desired configuration from the Visual Studio toolbar (e.g., `Debug` or `Release` and `x64`).
2.  **Build Solution:** From the menu bar, select `Build` -> `Build Solution` or press `Ctrl + Shift + B`.

This will compile `Engine.Core` first, then `Engine.Application`. You'll see output in the Output window indicating build progress.

### Running the Application

After a successful build:

1.  **Set Startup Project:** Right-click `Engine.Application` in Solution Explorer and select `Set as Startup Project`.
2.  **Run:** Press `F5` to run with debugging, or `Ctrl + F5` to run without debugging.

Initially, you'll see a basic Win32 window open, and console output from the `PluginManager` and any loaded plugins (like `Graphics.DX12` once you implement it).

---

## Core Modules

Here's a brief overview of the initial core modules:

* `Engine.Core`: Contains fundamental utilities, interfaces (`IPlugin`, `IGraphicsSystem`, `IAISystem`), and the `PluginManager` for dynamic module loading.
* `Engine.Application`: The host application that orchestrates the engine's lifecycle and manages the main game loop.
* `Engine.Graphics.DX12`: Our first hot-reloadable plugin, responsible for all rendering operations using DirectX 12. It implements the `IGraphicsSystem` interface.
* `Engine.AI`: (Planned) The dedicated plugin for all artificial intelligence behaviors, pathfinding, and decision-making systems, implementing `IAISystem`.
* `Engine.Editor`: (Planned) The separate application that uses the engine's core and plugins to provide a visual development environment.

---

## Hot Reloading

One of the cornerstone features of this engine is its hot-reloading capability. This allows you to:

1.  Make code changes within a plugin (e.g., `Engine.Graphics.DX12`).
2.  Compile *only that plugin's DLL*.
3.  The `PluginManager` will detect the file change, unload the old DLL, and load the new one *while the engine is still running*.

This dramatically speeds up development iteration times, especially for visual tweaks and gameplay logic. The system uses temporary "shadow copies" of DLLs to enable this seamless process.

---

## AI Subsystem

AI is a first-class citizen in Angaraka. The `Engine.AI` plugin will eventually house:

* **AI Agent Management:** Registration and updates for all AI entities.
* **Behavior Frameworks:** Support for Behavior Trees, State Machines, or custom decision-making systems.
* **Navigation:** Integrated pathfinding and navigation mesh generation/management (e.g., using Recast/Detour).
* **Perception:** Systems for AI agents to "see" and "hear" the game world.

---

## The Editor

The `Engine.Editor` will be your primary tool for content creation and engine interaction. It will:

* Provide a graphical user interface (initially powered by [Dear ImGui](https://github.com/ocornut/imgui)).
* Load and interact with the same engine plugins (`Engine.Graphics.DX12`, `Engine.AI`, etc.) to visualize and manipulate scene data.
* Offer debugging tools, property editors, and potentially specialized tools for AI behavior creation, level design, and more.
* Leverage a custom **reflection system** to automatically expose engine data and properties to the UI.

---

## Contributing

We welcome contributions to Angaraka! If you're interested in helping out, please:

1.  Fork the repository.
2.  Create a new branch for your feature or bug fix.
3.  Commit your changes following a clear and concise commit message convention.
4.  Push your branch and open a Pull Request.

Please ensure your code adheres to the project's coding style and includes appropriate tests if applicable.

---

## License

This project is licensed under the [MIT License](LICENSE) - see the `LICENSE` file for details.

---

## Contact

For any questions or discussions, please reach out via [GitHub Issues].

