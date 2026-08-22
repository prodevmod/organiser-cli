#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <filesystem>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <algorithm>
#include <windows.h>
#include <wincrypt.h>

namespace fs = std::filesystem;

const int STALE_DAYS = 30;

const std::string RED = "\033[91m";
const std::string GREEN = "\033[92m";
const std::string YELLOW = "\033[93m";
const std::string CYAN = "\033[96m";
const std::string RESET = "\033[0m";

const std::string BANNER = RED + R"(
  /$$$$$$                                           /$$                                                        
 /$$__  $$                                         |__/                                                        
| $$  \ $$  /$$$$$$   /$$$$$$   /$$$$$$  /$$$$$$$  /$$  /$$$$$$$  /$$$$$$   /$$$$$$   /$$$$$$$  /$$$$$$   /$$$$$$ 
| $$  | $$ /$$__  $$ /$$__  $$ |____  $$| $$__  $$| $$ /$$_____/ /$$__  $$ /$$__  $$ /$$_____/ /$$__  $$ /$$__  $$
| $$  | $$| $$  \__/| $$  \ $$  /$$$$$$$| $$  \ $$| $$|  $$$$$$ | $$$$$$$$| $$  \__/| $$      | $$  \ $$| $$  \ $$
| $$  | $$| $$      | $$  | $$ /$$__  $$| $$  | $$| $$ \____  $$| $$_____/| $$      | $$      | $$  | $$| $$  | $$
|  $$$$$$/| $$      |  $$$$$$$|  $$$$$$$| $$  | $$| $$ /$$$$$$$/|  $$$$$$$| $$ /$$  |  $$$$$$$| $$$$$$$/| $$$$$$$/
 \______/ |__/       \____  $$ \_______/|__/  |__/|__/|_______/  \_______/|__/|__/   \_______/| $$____/ | $$____/ 
                     /$$  \ $$                                                                | $$      | $$      
                    |  $$$$$$/                                                                | $$      | $$      
                     \______/                                                                 |__/      |__/      
)" + RESET;
const std::map<std::string, std::string> EXTENSION_MAP = {
    {".exe", "Apps/Setups_and_Maintainers"}, {".msi", "Apps/Setups_and_Maintainers"},
    {".dmg", "Apps/Setups_and_Maintainers"}, {".pkg", "Apps/Setups_and_Maintainers"},
    {".deb", "Apps/Setups_and_Maintainers"}, {".rpm", "Apps/Setups_and_Maintainers"},
    
    {".py", "Codes"}, {".pyw", "Codes"}, {".ipynb", "Codes"},
    {".cpp", "Codes"}, {".c", "Codes"}, {".h", "Codes"}, {".hpp", "Codes"}, {".cc", "Codes"},
    {".cs", "Codes"}, {".js", "Codes"}, {".ts", "Codes"}, {".jsx", "Codes"}, {".tsx", "Codes"},
    {".html", "Codes"}, {".css", "Codes"}, {".scss", "Codes"},
    {".json", "Codes"}, {".xml", "Codes"}, {".yaml", "Codes"}, {".yml", "Codes"},
    {".rs", "Codes"}, {".go", "Codes"}, {".java", "Codes"}, {".php", "Codes"},
    {".rb", "Codes"}, {".swift", "Codes"}, {".kt", "Codes"}, {".sql", "Codes"},
    {".sh", "Codes"}, {".bat", "Codes"}, {".cmd", "Codes"}, {".ps1", "Codes"},

    {".unitypackage", "Game_Engines"}, {".uproject", "Game_Engines"}, {".godot", "Game_Engines"},
    {".unity", "Game_Engines"}, {".prefab", "Game_Engines"}, {".tscn", "Game_Engines"},
    {".tres", "Game_Engines"}, {".asset", "Game_Engines"},

    {".obj", "3D_Models"}, {".fbx", "3D_Models"}, {".blend", "3D_Models"},
    {".stl", "3D_Models"}, {".gltf", "3D_Models"}, {".glb", "3D_Models"},
    {".dae", "3D_Models"}, {".ply", "3D_Models"}, {".3ds", "3D_Models"},
    {".step", "3D_Models"}, {".stp", "3D_Models"}, {".iges", "3D_Models"},

    {".pdf", "Documents"}, {".docx", "Documents"}, {".doc", "Documents"},
    {".txt", "Documents"}, {".xlsx", "Documents"}, {".csv", "Documents"},
    {".pptx", "Documents"}, {".md", "Documents"},

    {".png", "Images"}, {".jpg", "Images"}, {".jpeg", "Images"},
    {".svg", "Images"}, {".gif", "Images"}, {".webp", "Images"},
    {".ico", "Images"}, {".psd", "Images"}, {".ai", "Images"},

    {".zip", "Archives"}, {".tar", "Archives"}, {".gz", "Archives"},
    {".7z", "Archives"}, {".rar", "Archives"}, {".iso", "Archives"}, {".tgz", "Archives"},

    {".mp4", "Media"}, {".mp3", "Media"}, {".mkv", "Media"},
    {".wav", "Media"}, {".mov", "Media"}, {".avi", "Media"},
    {".flac", "Media"}, {".ogg", "Media"}
};

std::atomic<bool> is_running(false);
std::mutex cout_mutex;
HANDLE g_hDir = INVALID_HANDLE_VALUE;

void log_message(const std::string& message) {
    std::lock_guard<std::mutex> lock(cout_mutex);
    std::cout << "\r\033[K" << YELLOW << "[SYSTEM] " << RESET << message << "\n";
    std::cout << CYAN << "organiser> " << RESET << std::flush;
}

fs::path get_default_directory() {
    const char* user_profile = std::getenv("USERPROFILE");
    if (user_profile) {
        return fs::path(user_profile) / "Downloads";
    }
    return fs::current_path();
}

std::set<std::string> load_hashes(const fs::path& hash_file) {
    std::set<std::string> hashes;
    std::ifstream file(hash_file);
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) hashes.insert(line);
    }
    return hashes;
}

void save_hashes(const std::set<std::string>& hashes, const fs::path& hash_file) {
    std::ofstream file(hash_file);
    for (const auto& hash : hashes) {
        file << hash << "\n";
    }
}

std::string calculate_sha256(const fs::path& file_path) {
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    BYTE buffer[65536];
    DWORD bytesRead = 0;
    BYTE rgbHash[32];
    DWORD cbHash = 32;
    CHAR rgbDigits[] = "0123456789abcdef";
    std::string hash_str = "";

    hFile = CreateFileW(file_path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return "";

    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        CloseHandle(hFile);
        return "";
    }

    if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
        CryptReleaseContext(hProv, 0);
        CloseHandle(hFile);
        return "";
    }

    while (ReadFile(hFile, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead > 0) {
        CryptHashData(hHash, buffer, bytesRead, 0);
    }

    if (CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0)) {
        for (DWORD i = 0; i < cbHash; i++) {
            hash_str += rgbDigits[rgbHash[i] >> 4];
            hash_str += rgbDigits[rgbHash[i] & 0x0F];
        }
    }

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    CloseHandle(hFile);
    return hash_str;
}

fs::path get_unique_path(const fs::path& target_dir, const fs::path& original_filename) {
    fs::path target_path = target_dir / original_filename;
    if (!fs::exists(target_path)) {
        return target_path;
    }

    std::string stem = original_filename.stem().string();
    std::string ext = original_filename.extension().string();
    int counter = 1;

    while (fs::exists(target_path)) {
        std::string new_name = stem + "_" + std::to_string(counter) + ext;
        target_path = target_dir / new_name;
        counter++;
    }
    return target_path;
}

void process_file(const fs::path& file_path, const fs::path& watch_dir, std::set<std::string>& seen_hashes, const fs::path& hash_file) {
    std::error_code ec;
    if (!fs::exists(file_path, ec) || fs::is_directory(file_path, ec)) return;

    std::string filename = file_path.filename().string();
    std::string ext = file_path.extension().string();
    
    std::string ext_lower = ext;
    std::transform(ext_lower.begin(), ext_lower.end(), ext_lower.begin(), ::tolower);

    if (filename.rfind(".", 0) == 0 || ext_lower == ".crdownload" || ext_lower == ".tmp" || ext_lower == ".part") {
        return;
    }
    if (file_path.parent_path() != watch_dir) return;

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    std::string file_hash = calculate_sha256(file_path);
    if (file_hash.empty()) return;

    fs::path duplicates_dir = watch_dir / "Duplicates";
    if (seen_hashes.count(file_hash)) {
        fs::create_directories(duplicates_dir, ec);
        fs::path unique_target = get_unique_path(duplicates_dir, file_path.filename());
        fs::rename(file_path, unique_target, ec);
        log_message("Moved duplicate: " + filename + " -> Duplicates/" + unique_target.filename().string());
        return;
    }

    seen_hashes.insert(file_hash);
    save_hashes(seen_hashes, hash_file);

    std::string target_category = "Misc";
    auto it = EXTENSION_MAP.find(ext_lower);
    if (it != EXTENSION_MAP.end()) {
        target_category = it->second;
    }

    fs::path target_dir = watch_dir / target_category;
    fs::create_directories(target_dir, ec);
    
    fs::path unique_target = get_unique_path(target_dir, file_path.filename());
    fs::rename(file_path, unique_target, ec);
    
    log_message("Organized: " + filename + " -> " + target_category + "/" + unique_target.filename().string());
}

void scan_existing_files(const fs::path& watch_dir, std::set<std::string>& seen_hashes, const fs::path& hash_file) {
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(watch_dir, ec)) {
        if (entry.is_regular_file(ec)) {
            process_file(entry.path(), watch_dir, seen_hashes, hash_file);
        }
    }
}

void clean_stale_files(const fs::path& watch_dir) {
    fs::path archive_dir = watch_dir / "_Archive";
    std::error_code ec;
    auto now = std::chrono::system_clock::now();
    int count = 0;

    for (const auto& entry : fs::directory_iterator(watch_dir, ec)) {
        if (entry.is_directory(ec)) {
            std::string folder_name = entry.path().filename().string();
            if (folder_name == "_Archive" || folder_name == "Duplicates") continue;

            for (const auto& file : fs::recursive_directory_iterator(entry.path(), ec)) {
                if (file.is_regular_file(ec)) {
                    auto ftime = fs::last_write_time(file.path(), ec);
                    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                        ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
                    );

                    auto age_days = std::chrono::duration_cast<std::chrono::hours>(now - sctp).count() / 24;
                    if (age_days >= STALE_DAYS) {
                        fs::create_directories(archive_dir, ec);
                        fs::rename(file.path(), archive_dir / file.path().filename(), ec);
                        count++;
                    }
                }
            }
        }
    }
    if (count > 0) log_message("Archived " + std::to_string(count) + " stale files (>30 days).");
}

void prune_empty_folders(const fs::path& watch_dir) {
    std::vector<fs::path> dirs;
    std::error_code ec;

    for (const auto& entry : fs::recursive_directory_iterator(watch_dir, fs::directory_options::skip_permission_denied, ec)) {
        if (entry.is_directory(ec)) {
            std::string folder_name = entry.path().filename().string();
            if (folder_name != "_Archive" && folder_name != "Duplicates") {
                dirs.push_back(entry.path());
            }
        }
    }

    std::reverse(dirs.begin(), dirs.end());
    int empty_count = 0;

    for (const auto& dir : dirs) {
        if (fs::exists(dir, ec) && fs::is_empty(dir, ec)) {
            fs::remove(dir, ec);
            empty_count++;
        }
    }
    if (empty_count > 0) log_message("Pruned " + std::to_string(empty_count) + " empty folders.");
}

void watcher_worker(fs::path watch_dir, std::set<std::string> seen_hashes, fs::path hash_file) {
    g_hDir = CreateFileW(
        watch_dir.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL
    );

    if (g_hDir == INVALID_HANDLE_VALUE) {
        log_message("Error: Failed to open directory handle for monitoring.");
        is_running = false;
        return;
    }

    BYTE buffer[1024];
    DWORD bytesReturned;
    auto last_stale_check = std::chrono::steady_clock::now();

    while (is_running) {
        if (ReadDirectoryChangesW(
            g_hDir, buffer, sizeof(buffer), FALSE,
            FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
            &bytesReturned, NULL, NULL)) 
        {
            if (!is_running) break;
            
            FILE_NOTIFY_INFORMATION* pNotify = (FILE_NOTIFY_INFORMATION*)buffer;
            while (pNotify) {
                std::wstring wFilename(pNotify->FileName, pNotify->FileNameLength / sizeof(WCHAR));
                fs::path changed_file = watch_dir / wFilename;
                
                process_file(changed_file, watch_dir, seen_hashes, hash_file);

                if (pNotify->NextEntryOffset == 0) break;
                pNotify = (FILE_NOTIFY_INFORMATION*)((BYTE*)pNotify + pNotify->NextEntryOffset);
            }
        } else {
            break;
        }

        auto current_time = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::hours>(current_time - last_stale_check).count() >= 24) {
            clean_stale_files(watch_dir);
            prune_empty_folders(watch_dir);
            last_stale_check = current_time;
        }
    }

    CloseHandle(g_hDir);
    g_hDir = INVALID_HANDLE_VALUE;
}

void print_help() {
    std::cout << "\n" << CYAN << "Available Commands:" << RESET << "\n";
    std::cout << "  " << GREEN << "start" << RESET << "       - Start watching the target folder\n";
    std::cout << "  " << RED << "stop" << RESET << "        - Stop watching the folder\n";
    std::cout << "  " << YELLOW << "status" << RESET << "      - Check if the watcher is running\n";
    std::cout << "  " << YELLOW << "path" << RESET << "        - View current target path or change it (e.g., path C:\\Folder)\n";
    std::cout << "  " << YELLOW << "scan" << RESET << "        - Manually organize existing files right now\n";
    std::cout << "  " << YELLOW << "clean" << RESET << "       - Force archive files older than 30 days\n";
    std::cout << "  " << YELLOW << "prune" << RESET << "       - Delete all empty folders in the target directory\n";
    std::cout << "  " << CYAN << "help" << RESET << "        - Show this menu\n";
    std::cout << "  " << RED << "exit" << RESET << "        - Close the program\n\n";
}

int main() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    if (GetConsoleMode(hOut, &dwMode)) {
        SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }

    std::cout << BANNER << "\n";
    std::cout << "Welcome to the C++ File Hygiene Daemon.\n";

    fs::path watch_dir = get_default_directory();
    fs::path hash_file = watch_dir / ".seen_hashes.txt";
    std::set<std::string> seen_hashes = load_hashes(hash_file);

    print_help();

    std::string cmd;
    std::thread worker;

    while (true) {
        std::cout << CYAN << "organiser> " << RESET << std::flush;
        if (!std::getline(std::cin, cmd)) break;

        cmd.erase(cmd.begin(), std::find_if(cmd.begin(), cmd.end(), [](unsigned char ch) { return !std::isspace(ch); }));
        cmd.erase(std::find_if(cmd.rbegin(), cmd.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), cmd.end());

        if (cmd == "help") {
            print_help();
        }
        else if (cmd == "start") {
            if (is_running) {
                std::cout << YELLOW << "Watcher is already running!\n" << RESET;
            } else {
                is_running = true;
                worker = std::thread(watcher_worker, watch_dir, seen_hashes, hash_file);
                std::cout << GREEN << "Watcher started on: " << watch_dir.string() << "\n" << RESET;
            }
        }
        else if (cmd == "stop") {
            if (!is_running) {
                std::cout << YELLOW << "Watcher is not running.\n" << RESET;
            } else {
                is_running = false;
                if (g_hDir != INVALID_HANDLE_VALUE) {
                    CancelIoEx(g_hDir, NULL);
                }
                if (worker.joinable()) {
                    worker.join();
                }
                std::cout << RED << "Watcher stopped.\n" << RESET;
            }
        }
        else if (cmd == "status") {
            std::cout << "Status: " << (is_running ? GREEN + "RUNNING" : RED + "STOPPED") << RESET << "\n";
            std::cout << "Target Directory: " << watch_dir.string() << "\n";
        }
        else if (cmd == "path") {
            std::cout << CYAN << "Current Target Path: " << RESET << watch_dir.string() << "\n";
            std::cout << "To change it, type: " << YELLOW << "path <absolute_folder_path>\n" << RESET;
        }
        else if (cmd.rfind("path ", 0) == 0 || cmd.rfind("PATH ", 0) == 0) {
            if (is_running) {
                std::cout << YELLOW << "Please 'stop' the watcher before changing target directories.\n" << RESET;
            } else {
                std::string new_path_str = cmd.substr(5);
                if (!new_path_str.empty() && (new_path_str.front() == '"' || new_path_str.front() == '\'')) {
                    new_path_str.erase(0, 1);
                }
                if (!new_path_str.empty() && (new_path_str.back() == '"' || new_path_str.back() == '\'')) {
                    new_path_str.pop_back();
                }

                fs::path new_path = fs::absolute(new_path_str);
                std::error_code ec;
                if (fs::exists(new_path, ec) && fs::is_directory(new_path, ec)) {
                    watch_dir = new_path;
                    hash_file = watch_dir / ".seen_hashes.txt";
                    seen_hashes = load_hashes(hash_file);
                    std::cout << GREEN << "Successfully re-targeted directory to: " << watch_dir.string() << "\n" << RESET;
                } else {
                    std::cout << RED << "Error: Invalid path or directory does not exist.\n" << RESET;
                }
            }
        }
        else if (cmd == "scan") {
            std::cout << "Scanning target directory...\n";
            scan_existing_files(watch_dir, seen_hashes, hash_file);
            std::cout << GREEN << "Scan complete.\n" << RESET;
        }
        else if (cmd == "clean") {
            std::cout << "Archiving old files...\n";
            clean_stale_files(watch_dir);
            std::cout << GREEN << "Cleanup complete.\n" << RESET;
        }
        else if (cmd == "prune") {
            std::cout << "Hunting for empty folders...\n";
            prune_empty_folders(watch_dir);
            std::cout << GREEN << "Pruning complete.\n" << RESET;
        }
        else if (cmd == "exit" || cmd == "quit") {
            if (is_running) {
                is_running = false;
                if (g_hDir != INVALID_HANDLE_VALUE) CancelIoEx(g_hDir, NULL);
                if (worker.joinable()) worker.join();
            }
            std::cout << RED << "Shutting down... Goodbye!\n" << RESET;
            break;
        }
        else if (!cmd.empty()) {
            std::cout << "Unknown command: '" << cmd << "'. Type 'help' for options.\n";
        }
    }

    return 0;
}