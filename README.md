# ORGANISER-CLI
(the fact that it is a cli is purely stylistic (**bash toolbox nostalgia**))
<img width="764" height="227" alt="image" src="https://github.com/user-attachments/assets/b6d94f6e-7f07-4d53-abf0-0852cfe55f45" />

## Python ex.
<img width="664" height="327" alt="image" src="https://github.com/user-attachments/assets/a424a727-64fe-497c-9bc6-74c71e0a7ebc" />

## C++ ex.
<img width="923" height="516" alt="image" src="https://github.com/user-attachments/assets/82eeb15e-7943-4a49-a0f4-0d05b6a3bb69" />


## The meaning of this PROJECT for me
My first real world application of my project and I am actually proud of it. First of all it solves a real problem of mine which is organising 
(I always want to be organised **especialy when it comes to folders** but can never seem to keep track of anything and everything turns out to be messy) 
while not bloating my pc or occupying a large part of my cpu (at least when it comes to the c++ version) and being incredibly fast. Moreover 
I actually got to use concept like hashing that considered useless (rly) because I didn't have any prior coding experience (in which I optimised performance).

## Installation
### Using Releases (recommended)
  * Navigate to the **[Releases](https://github.com/prodevmod/organiser-cli/releases)** section.
  * Click on the latest release.
  * Download organiser.exe (C++)(recommended) or organiserpy.exe (Python) by clicking on either.
  * Place the executable in your preferred directory or add it to your system PATH.
  * Run the executable.

### Compiling from Source (u're weird)
#### Building the C++ (recommended)
##### MinGW / g++
`g++ ./cpp/organiser.cpp -o ./executables/organiser.exe -O3 -lcrypt32`

##### MSVC
`cl /EHsc /O2 /Fe:.\executables\organiser.exe .\cpp\organiser.cpp Advapi32.lib`

#### Building the Python
`pip install watchdog pyinstaller`

`pyinstaller --onefile --name organiserpy --distpath ./executables ./python/organiser.py`

## CLI Commands

| Command | Description |
| :--- | :--- |
| `start` | Begins real-time directory monitoring in the background. |
| `stop` | Halts the directory watcher thread/observer. |
| `status` | Checks active status and targets directory path. |
| `path` | Displays the target directory path. |
| `path <dir_path>` | Changes the target folder (requires watcher to be stopped first). |
| `stats` | Shows storage analytics and category breakdowns. |
| `scan` | Instantly processes all un-organized files currently in the directory. |
| `clean` | Archives files older than 30 days into `_Archive/`. |
| `prune` | Safely removes empty folders recursively. |
| `custom delete <keyword>` | Deletes all files matching a keyword or extension (e.g., `.tmp`). |
| `custom dir <folder> <kw>`| Moves all files matching a keyword into a targeted subfolder. |
| `help` | Prints the interactive command menu. |
| `exit` | Gracefully terminates the application. |

## Y C++ over Python tho
Both implementations share identical command structures and functionality:

| Feature | C++ Executable (`organiser.exe`) | Python Executable (`organiserpy.exe`) |
| :--- | :--- | :--- |
| **Performance** | Instantaneous / Native | Micro-delay on cold start |
| **Binary Size** | **~265 KB**  *(Recommended)* | **~7.2 MB** |
| **Dependencies** | None (Native Windows APIs) | PyInstaller / Watchdog runtime bundle |

## A deeper look into categories
Apps/              (Windows, Mac, Linux installers)

Codes/             (Python, CPP, Web, Data, Scripts, etc.)

Game_Engines/      (Unity, Unreal, Godot assets)

3D_Models/         (Blender, OBJ, FBX, STL, CAD)

Documents/         (PDF, Word, Excel, Markdown)

Images/            (PNG, JPEG, Vector, Photoshop)

Archives/          (ZIP, RAR, 7Z, ISO)

Media/             (Audio, Video)

Duplicates/        (SHA-256 duplicate matches)

_Archive/          (Stale files older than 30 days)

# PLS STAR THIS AMAZING REVOLUTIONARY PROJECT/REPOSITORY

# Author
Rosario Alexandros Morabito
