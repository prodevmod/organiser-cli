import os
import time
import pathlib
import hashlib
import shutil
import json
import threading
import sys
from datetime import datetime, timedelta
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler

# --- TERMINAL COLORS ---
RED = "\033[91m"
GREEN = "\033[92m"
YELLOW = "\033[93m"
CYAN = "\033[96m"
RESET = "\033[0m"

BANNER = RED + r"""
  /$$$$$$                                          /$$                                          
 /$$__  $$                                        |__/                                          
| $$  \ $$  /$$$$$$   /$$$$$$   /$$$$$$  /$$$$$$$  /$$  /$$$$$$$  /$$$$$$   /$$$$$$   /$$$$$$  /$$   /$$
| $$  | $$ /$$__  $$ /$$__  $$ |____  $$| $$__  $$| $$ /$$_____/ /$$__  $$ /$$__  $$ /$$__  $$| $$  | $$
| $$  | $$| $$  \__/| $$  \ $$  /$$$$$$$| $$  \ $$| $$|  $$$$$$ | $$$$$$$$| $$  \__/| $$  \ $$| $$  | $$
| $$  | $$| $$      | $$  | $$ /$$__  $$| $$  | $$| $$ \____  $$| $$_____/| $$      | $$  | $$| $$  | $$
|  $$$$$$/| $$      |  $$$$$$$|  $$$$$$$| $$  | $$| $$ /$$$$$$$/|  $$$$$$$| $$ /$$  | $$$$$$$/|  $$$$$$$
 \______/ |__/       \____  $$ \_______/|__/  |__/|__/|_______/  \_______/|__/|__/  | $$____/  \____  $$
                     /$$  \ $$                                                      | $$       /$$  | $$
                    |  $$$$$$/                                                      | $$      |  $$$$$$/
                     \______/                                                       |__/       \______/ """ + RESET

# Expanded categories with subfolders
EXT_MAP = {
    # Apps
    ".exe": "Apps/Windows", ".msi": "Apps/Windows",
    ".dmg": "Apps/Mac", ".pkg": "Apps/Mac",
    ".deb": "Apps/Linux", ".rpm": "Apps/Linux",
    
    # Codes
    ".py": "Codes/Python", ".pyw": "Codes/Python", ".ipynb": "Codes/Python",
    ".cpp": "Codes/CPP", ".c": "Codes/C", ".h": "Codes/C_CPP_Headers", ".hpp": "Codes/C_CPP_Headers", ".cc": "Codes/CPP",
    ".cs": "Codes/CSharp", ".js": "Codes/JavaScript", ".ts": "Codes/TypeScript", ".jsx": "Codes/React", ".tsx": "Codes/React",
    ".html": "Codes/Web", ".css": "Codes/Web", ".scss": "Codes/Web",
    ".json": "Codes/Data", ".xml": "Codes/Data", ".yaml": "Codes/Data", ".yml": "Codes/Data",
    ".rs": "Codes/Rust", ".go": "Codes/Go", ".java": "Codes/Java", ".php": "Codes/PHP",
    ".rb": "Codes/Ruby", ".swift": "Codes/Swift", ".kt": "Codes/Kotlin", ".sql": "Codes/Database",
    ".sh": "Codes/Scripts", ".bat": "Codes/Scripts", ".cmd": "Codes/Scripts", ".ps1": "Codes/Scripts",

    # Game Engines
    ".unitypackage": "Game_Engines/Unity", ".unity": "Game_Engines/Unity", ".prefab": "Game_Engines/Unity", ".asset": "Game_Engines/Unity",
    ".uproject": "Game_Engines/Unreal", 
    ".godot": "Game_Engines/Godot", ".tscn": "Game_Engines/Godot", ".tres": "Game_Engines/Godot",

    # 3D Models
    ".obj": "3D_Models/OBJ", ".fbx": "3D_Models/FBX", ".blend": "3D_Models/Blender",
    ".stl": "3D_Models/STL", ".gltf": "3D_Models/GLTF", ".glb": "3D_Models/GLTF",
    ".dae": "3D_Models/Collada", ".ply": "3D_Models/PLY", ".3ds": "3D_Models/3DS",
    ".step": "3D_Models/CAD", ".stp": "3D_Models/CAD", ".iges": "3D_Models/CAD",

    # Documents
    ".pdf": "Documents/PDF", ".docx": "Documents/Word", ".doc": "Documents/Word",
    ".txt": "Documents/Text", ".xlsx": "Documents/Excel", ".csv": "Documents/Data",
    ".pptx": "Documents/PowerPoint", ".md": "Documents/Markdown",

    # Images
    ".png": "Images/PNG", ".jpg": "Images/JPEG", ".jpeg": "Images/JPEG",
    ".svg": "Images/Vector", ".gif": "Images/GIF", ".webp": "Images/WebP",
    ".ico": "Images/Icons", ".psd": "Images/Photoshop", ".ai": "Images/Illustrator",

    # Archives
    ".zip": "Archives/ZIP", ".tar": "Archives/TAR", ".gz": "Archives/GZ",
    ".7z": "Archives/7Z", ".rar": "Archives/RAR", ".iso": "Archives/ISO", ".tgz": "Archives/TAR",

    # Media
    ".mp4": "Media/Video", ".mkv": "Media/Video", ".mov": "Media/Video", ".avi": "Media/Video",
    ".mp3": "Media/Audio", ".wav": "Media/Audio", ".flac": "Media/Audio", ".ogg": "Media/Audio"
}

class OrganizerHandler(FileSystemEventHandler):
    def __init__(self, cli_instance):
        self.cli = cli_instance
        
    def on_created(self, event):
        if not event.is_directory:
            self.cli.process_file(pathlib.Path(event.src_path))
            
    def on_moved(self, event):
        if not event.is_directory:
            self.cli.process_file(pathlib.Path(event.dest_path))

class CLI:
    def __init__(self):
        self.is_running = False
        self.observer = None
        self.worker_thread = None
        
        # Thread safety lock for the hashes set and JSON file
        self.lock = threading.Lock()
        
        # Default starting directory
        user_profile = os.environ.get("USERPROFILE") or os.environ.get("HOME")
        if user_profile:
            self.watch_dir = pathlib.Path(user_profile) / "Downloads"
        else:
            self.watch_dir = pathlib.Path.cwd()
            
        self.hash_file = self.watch_dir / ".seen_hashes.json"
        self.seen_hashes = set()
        self.load_hashes()

    def load_hashes(self):
        with self.lock:
            self.seen_hashes.clear()
            if self.hash_file.exists():
                try:
                    with open(self.hash_file, "r") as f:
                        self.seen_hashes = set(json.load(f))
                except Exception:
                    self.log("Failed to load hash database. Starting fresh.")

    def save_hashes(self):
        with self.lock:
            try:
                with open(self.hash_file, "w") as f:
                    json.dump(list(self.seen_hashes), f)
            except Exception:
                pass

    def get_sha256(self, file_path):
        hasher = hashlib.sha256()
        try:
            with open(file_path, "rb") as f:
                while chunk := f.read(65536):
                    hasher.update(chunk)
            return hasher.hexdigest()
        except Exception:
            return ""

    def log(self, message):
        sys.stdout.write(f"\r\033[K{YELLOW}[SYSTEM]{RESET} {message}\n")
        sys.stdout.write(f"{CYAN}organiser>{RESET} ")
        sys.stdout.flush()

    def wait_for_file_completion(self, file_path, retries=5, delay=1):
        """Wait until a file stops growing (useful for active downloads)."""
        previous_size = -1
        for _ in range(retries):
            try:
                current_size = file_path.stat().st_size
                if current_size == previous_size and current_size > 0:
                    return True
                previous_size = current_size
            except FileNotFoundError:
                return False
            time.sleep(delay)
        return False

    def get_unique_path(self, target_dir, filename):
        """Generates a unique path by appending _1, _2, etc. if the file exists."""
        target_path = target_dir / filename
        if not target_path.exists():
            return target_path
            
        stem = target_path.stem
        ext = target_path.suffix
        counter = 1
        while target_path.exists():
            target_path = target_dir / f"{stem}_{counter}{ext}"
            counter += 1
        return target_path

    def process_file(self, file_path):
        # Ignore hidden files, temporary downloads, or directories
        if file_path.name.startswith(".") or file_path.suffix.lower() in {".crdownload", ".tmp", ".part"}:
            return
        if file_path.parent != self.watch_dir or not file_path.exists() or file_path.is_dir():
            return

        # Ensure the file has finished downloading before hashing
        if not self.wait_for_file_completion(file_path):
            return

        file_hash = self.get_sha256(file_path)
        if not file_hash:
            return

        # Check for duplicates safely
        with self.lock:
            is_duplicate = file_hash in self.seen_hashes
            if not is_duplicate:
                self.seen_hashes.add(file_hash)

        duplicates_dir = self.watch_dir / "Duplicates"
        
        # Handle duplicate files
        if is_duplicate:
            duplicates_dir.mkdir(exist_ok=True)
            unique_target = self.get_unique_path(duplicates_dir, file_path.name)
            try:
                shutil.move(str(file_path), unique_target)
                self.log(f"Moved duplicate: {file_path.name} -> Duplicates/{unique_target.name}")
            except Exception as e:
                self.log(f"Failed to move duplicate {file_path.name}: {e}")
            return

        # If it's a new file, save the hash database
        self.save_hashes()

        # Determine target category folder using the dictionary
        target_category = EXT_MAP.get(file_path.suffix.lower(), "Misc")
        target_folder = self.watch_dir / target_category

        target_folder.mkdir(parents=True, exist_ok=True)
        unique_target = self.get_unique_path(target_folder, file_path.name)
        try:
            shutil.move(str(file_path), unique_target)
            self.log(f"Organized: {file_path.name} -> {target_category}/{unique_target.name}")
        except Exception as e:
            self.log(f"Failed to organize {file_path.name}: {e}")

    def clean_stale_files(self, days=30):
        archive_dir = self.watch_dir / "_Archive"
        cutoff = datetime.now() - timedelta(days=days)
        count = 0

        for item in self.watch_dir.iterdir():
            # Skip special folders
            if item.is_dir() and item.name not in {"_Archive", "Duplicates"} and not item.name.startswith("."):
                for path in item.rglob("*"):
                    if path.is_file() and datetime.fromtimestamp(path.stat().st_mtime) < cutoff:
                        try:
                            archive_dir.mkdir(exist_ok=True)
                            unique_target = self.get_unique_path(archive_dir, path.name)
                            shutil.move(str(path), unique_target)
                            count += 1
                        except Exception:
                            pass
                            
        if count > 0:
            self.log(f"Cleanup complete. Archived {count} old files.")
        else:
            self.log("No stale files found to archive.")

    def prune_empty_folders(self):
        empty_count = 0
        for dirpath, dirnames, filenames in os.walk(self.watch_dir, topdown=False):
            path = pathlib.Path(dirpath)
            
            if path == self.watch_dir:
                continue
            if path.name in {"_Archive", "Duplicates"} or path.name.startswith("."):
                continue
                
            try:
                if not any(path.iterdir()):
                    path.rmdir()
                    empty_count += 1
            except Exception:
                pass
                
        if empty_count > 0:
            self.log(f"Pruning complete. Erased {empty_count} empty folders!")
        else:
            self.log("No empty folders found to prune.")

    def print_help(self):
        print(f"\n{CYAN}Available Commands:{RESET}")
        print(f"  {GREEN}start{RESET}                 - Start watching the target directory")
        print(f"  {RED}stop{RESET}                  - Stop watching the directory")
        print(f"  {YELLOW}status{RESET}                - Check if the watcher is running & current path")
        print(f"  {YELLOW}path{RESET}                  - View current target path or change it (e.g., path C:\\Folder)")
        print(f"  {YELLOW}scan{RESET}                  - Manually organize existing files right now")
        print(f"  {YELLOW}clean{RESET}                 - Force archive files older than 30 days")
        print(f"  {YELLOW}prune{RESET}                 - Delete all empty folders in the target directory")
        print(f"  {YELLOW}custom delete <word>{RESET}  - Deletes all files recursively containing the word or .extension")
        print(f"  {YELLOW}custom dir <dir> <kw>{RESET} - Groups all files recursively containing the keyword into a new folder")
        print(f"  {CYAN}help{RESET}                  - Show this menu")
        print(f"  {RED}exit{RESET}                  - Close the program\n")

    def run_watcher(self):
        self.observer = Observer()
        self.observer.schedule(OrganizerHandler(self), str(self.watch_dir), recursive=False)
        self.observer.start()
        
        while self.is_running:
            time.sleep(1)
            
        self.observer.stop()
        self.observer.join()

    def start(self):
        print(BANNER)
        print("Welcome to the Python File Hygiene Daemon.")
        self.print_help()
        
        while True:
            try:
                cmd = input(f"{CYAN}organiser>{RESET} ").strip()
                
                if not cmd:
                    continue
                    
                if cmd.lower() == "help":
                    self.print_help()
                    
                elif cmd.lower() == "start":
                    if self.is_running:
                        print(f"{YELLOW}Watcher is already running!{RESET}")
                    else:
                        self.is_running = True
                        self.worker_thread = threading.Thread(target=self.run_watcher, daemon=True)
                        self.worker_thread.start()
                        print(f"{GREEN}Watcher started on: {self.watch_dir}{RESET}")
                        
                elif cmd.lower() == "stop":
                    if not self.is_running:
                        print(f"{YELLOW}Watcher is not running.{RESET}")
                    else:
                        self.is_running = False
                        print(f"{RED}Watcher stopped.{RESET}")
                        
                elif cmd.lower() == "status":
                    state = f"{GREEN}RUNNING{RESET}" if self.is_running else f"{RED}STOPPED{RESET}"
                    print(f"Status: {state}")
                    print(f"Target Directory: {self.watch_dir}")
                    
                elif cmd.lower() == "path":
                    print(f"{CYAN}Current Target Path:{RESET} {self.watch_dir}")
                    print(f"To change it, type: {YELLOW}path <absolute_folder_path>{RESET}")
                    
                elif cmd.lower().startswith("path "):
                    if self.is_running:
                        print(f"{YELLOW}Please 'stop' the watcher before changing target directories.{RESET}")
                    else:
                        new_path_str = cmd[5:].strip().strip('"').strip("'")
                        new_path = pathlib.Path(new_path_str).resolve()
                        if new_path.is_dir():
                            self.watch_dir = new_path
                            self.hash_file = self.watch_dir / ".seen_hashes.json"
                            self.load_hashes()
                            print(f"{GREEN}Successfully re-targeted directory to: {self.watch_dir}{RESET}")
                        else:
                            print(f"{RED}Error: Invalid path or directory does not exist.{RESET}")
                            
                elif cmd.lower().startswith("custom delete "):
                    keyword = cmd[14:].strip()
                    if not keyword:
                        print(f"{RED}Error: Provide a keyword/extension (e.g., custom delete .tmp){RESET}")
                    else:
                        del_count = 0
                        for path in self.watch_dir.rglob("*"):
                            if path.is_file() and keyword in path.name:
                                try:
                                    path.unlink()
                                    del_count += 1
                                except Exception:
                                    pass
                        print(f"{GREEN}Deleted {del_count} files containing '{keyword}'.{RESET}")
                        
                elif cmd.lower().startswith("custom dir "):
                    args = cmd[11:].strip().split(maxsplit=1)
                    if len(args) == 2:
                        folder_name, keyword = args
                        custom_path = self.watch_dir / folder_name
                        custom_path.mkdir(exist_ok=True)
                        
                        move_count = 0
                        # Pre-collect to prevent modifying the directory while iterating
                        to_move = [
                            p for p in self.watch_dir.rglob("*") 
                            if p.is_file() and keyword in p.name and custom_path not in p.parents
                        ]
                        
                        for path in to_move:
                            unique_target = self.get_unique_path(custom_path, path.name)
                            try:
                                shutil.move(str(path), unique_target)
                                move_count += 1
                            except Exception:
                                pass
                        print(f"{GREEN}Grouped {move_count} files containing '{keyword}' into '{folder_name}'.{RESET}")
                    else:
                        print(f"{RED}Error: Invalid format. Use: custom dir <FolderName> <keyword>{RESET}")

                elif cmd.lower() == "scan":
                    print("Scanning target directory...")
                    count = 0
                    for item in self.watch_dir.iterdir():
                        if item.is_file():
                            self.process_file(item)
                            count += 1
                    print(f"{GREEN}Scan complete. Checked {count} files.{RESET}")
                    
                elif cmd.lower() == "clean":
                    print("Archiving old files...")
                    self.clean_stale_files(30)
                    
                elif cmd.lower() == "prune":
                    print("Hunting for empty folders...")
                    self.prune_empty_folders()
                    
                elif cmd.lower() in ["exit", "quit"]:
                    self.is_running = False
                    print(f"{RED}Shutting down... Goodbye!{RESET}")
                    break
                    
                else:
                    print(f"Unknown command: '{cmd}'. Type 'help' for options.")
                    
            except KeyboardInterrupt:
                self.is_running = False
                print(f"\n{RED}Force quitting...{RESET}")
                break

if __name__ == "__main__":
    app = CLI()
    app.start()