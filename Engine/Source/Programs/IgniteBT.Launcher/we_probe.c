/*
 * we_probe — native no-op probe for IgniteBT (no CoreCLR).
 * Exit 0 = no-op, 2 = work required, 1 = error.
 */
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#define access _access
#else
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#define SNAPSHOT_MAGIC 0x4E534749u /* "IGSN" */
#define SNAPSHOT_V2 2
#define SNAPSHOT_V1 1
#define EXIT_NOOP 0
#define EXIT_ERROR 1
#define EXIT_BUILD 2
#define MAX_PATH_LEN 4096
#define MAX_FILES 4096
#define MAX_BUILDCS 256

typedef struct {
    char config[64];
    char platform[64];
    char target[128];
    char build_flags_hash[32];
    char manifest_config_hash[32];
    char manifest_toolchain_hash[32];
    char compiler_path[MAX_PATH_LEN];
    int64_t compiler_mtime;
    int64_t compiler_size;
    int jobs;
    int unity_build;
    int unity_size;
    char unity_disabled[512];
    int build_cs_count;
    char build_cs_paths[MAX_BUILDCS][MAX_PATH_LEN];
    int file_count;
    char file_paths[MAX_FILES][MAX_PATH_LEN];
    int64_t file_sizes[MAX_FILES];
    int64_t file_mtimes[MAX_FILES];
} snapshot_t;

typedef struct {
    char config[64];
    char platform[64];
    char target[128];
    int jobs;
    int unity_build;
    int unity_size;
    char unity_disabled[512];
    char project_root[MAX_PATH_LEN];
    char engine_root[MAX_PATH_LEN];
    int has_clean;
} build_args_t;

static int ieq(const char* a, const char* b) {
    if (!a || !b) return 0;
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        a++; b++;
    }
    return *a == *b;
}

static int read_u32(FILE* f, uint32_t* v) {
    return fread(v, 4, 1, f) == 1 ? 0 : -1;
}

static int read_i32(FILE* f, int32_t* v) {
    return fread(v, 4, 1, f) == 1 ? 0 : -1;
}

static int read_i64(FILE* f, int64_t* v) {
    return fread(v, 8, 1, f) == 1 ? 0 : -1;
}

static int read_string(FILE* f, char* out, size_t cap) {
    int32_t len;
    if (read_i32(f, &len) != 0 || len < 0 || (size_t)len >= cap) return -1;
    if (len > 0 && fread(out, 1, (size_t)len, f) != (size_t)len) return -1;
    out[len] = 0;
    return 0;
}

static int find_project_root(const char* start, char* out, size_t cap) {
    char path[MAX_PATH_LEN];
    char test[MAX_PATH_LEN];
#ifdef _WIN32
    if (!GetFullPathNameA(start, (DWORD)cap, path, NULL)) return -1;
#else
    if (!realpath(start, path)) strncpy(path, start, cap - 1);
#endif
    for (;;) {
        snprintf(test, sizeof(test), "%s/Engine/Source", path);
#ifdef _WIN32
        DWORD attr = GetFileAttributesA(test);
        if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY)) {
            snprintf(out, cap, "%s", path);
            return 0;
        }
        char* slash = strrchr(path, '\\');
        if (!slash) slash = strrchr(path, '/');
        if (!slash || slash == path) break;
        *slash = 0;
#else
        struct stat st;
        if (stat(test, &st) == 0 && S_ISDIR(st.st_mode)) {
            snprintf(out, cap, "%s", path);
            return 0;
        }
        char* slash = strrchr(path, '/');
        if (!slash || slash == path) break;
        *slash = 0;
#endif
    }
    return -1;
}

static void normalize_platform(const char* in, char* out, size_t cap) {
    if (!in || !*in) {
#ifdef _WIN32
        strncpy(out, "Windows", cap - 1);
#else
        strncpy(out, "Linux", cap - 1);
#endif
        return;
    }
    if (ieq(in, "win64") || ieq(in, "windows") || ieq(in, "win")) strncpy(out, "Windows", cap - 1);
    else if (ieq(in, "linux")) strncpy(out, "Linux", cap - 1);
    else if (ieq(in, "mac") || ieq(in, "macos")) strncpy(out, "Mac", cap - 1);
    else snprintf(out, cap, "%s", in);
}

static void config_folder(const char* config, char* out, size_t cap) {
    if (ieq(config, "development") || ieq(config, "dev")) strncpy(out, "Development", cap - 1);
    else if (ieq(config, "shipping") || ieq(config, "release")) strncpy(out, "Shipping", cap - 1);
    else if (ieq(config, "profile")) strncpy(out, "Profile", cap - 1);
    else strncpy(out, "Debug", cap - 1);
}

static int parse_build_args(int argc, char** argv, build_args_t* args) {
    memset(args, 0, sizeof(*args));
    strncpy(args->config, "Release", sizeof(args->config) - 1);
    strncpy(args->target, "Default", sizeof(args->target) - 1);
    args->jobs = 0;
#ifdef _WIN32
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    args->jobs = (int)si.dwNumberOfProcessors;
#else
    args->jobs = (int)sysconf(_SC_NPROCESSORS_ONLN);
#endif
    normalize_platform("", args->platform, sizeof(args->platform));

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--clean") == 0 || strcmp(argv[i], "-C") == 0) args->has_clean = 1;
        else if (strcmp(argv[i], "--unity") == 0) args->unity_build = 1;
        else if (strncmp(argv[i], "--config=", 9) == 0) snprintf(args->config, sizeof(args->config), "%s", argv[i] + 9);
        else if (strcmp(argv[i], "--config") == 0 || strcmp(argv[i], "-c") == 0) {
            if (i + 1 < argc) snprintf(args->config, sizeof(args->config), "%s", argv[++i]);
        } else if (strncmp(argv[i], "--platform=", 11) == 0) normalize_platform(argv[i] + 11, args->platform, sizeof(args->platform));
        else if (strcmp(argv[i], "--platform") == 0 || strcmp(argv[i], "-p") == 0) {
            if (i + 1 < argc) normalize_platform(argv[++i], args->platform, sizeof(args->platform));
        } else if (strncmp(argv[i], "--target=", 9) == 0) snprintf(args->target, sizeof(args->target), "%s", argv[i] + 9);
        else if (strcmp(argv[i], "--target") == 0 || strcmp(argv[i], "-t") == 0) {
            if (i + 1 < argc) snprintf(args->target, sizeof(args->target), "%s", argv[++i]);
        } else if (strncmp(argv[i], "--jobs=", 7) == 0) args->jobs = atoi(argv[i] + 7);
        else if (strcmp(argv[i], "--jobs") == 0 || strcmp(argv[i], "-j") == 0) {
            if (i + 1 < argc) args->jobs = atoi(argv[++i]);
        } else if (strncmp(argv[i], "--unity-size=", 13) == 0) args->unity_size = atoi(argv[i] + 13);
        else if (strncmp(argv[i], "--unity-disable=", 16) == 0) snprintf(args->unity_disabled, sizeof(args->unity_disabled), "%s", argv[i] + 16);
        else if (argv[i][0] != '-' && args->target[0] == 'D' && strcmp(args->target, "Default") == 0)
            snprintf(args->target, sizeof(args->target), "%s", argv[i]);
    }

    char cwd[MAX_PATH_LEN];
#ifdef _WIN32
    if (GetCurrentDirectoryA((DWORD)sizeof(cwd), cwd) == 0) return -1;
#else
    if (!getcwd(cwd, sizeof(cwd))) return -1;
#endif
    if (find_project_root(cwd, args->project_root, sizeof(args->project_root)) != 0) return -1;
    snprintf(args->engine_root, sizeof(args->engine_root), "%s/Engine", args->project_root);
    return 0;
}

static int load_snapshot(const char* path, snapshot_t* snap) {
    FILE* f = fopen(path, "rb");
    uint32_t magic;
    uint8_t version;
    if (!f) return -1;
    memset(snap, 0, sizeof(*snap));
    if (read_u32(f, &magic) != 0 || magic != SNAPSHOT_MAGIC) { fclose(f); return -1; }
    if (fread(&version, 1, 1, f) != 1) { fclose(f); return -1; }
    if (version != SNAPSHOT_V1 && version != SNAPSHOT_V2) { fclose(f); return -1; }

    if (read_string(f, snap->config, sizeof(snap->config)) != 0) goto fail;
    if (read_string(f, snap->platform, sizeof(snap->platform)) != 0) goto fail;
    if (read_string(f, snap->target, sizeof(snap->target)) != 0) goto fail;
    if (read_string(f, snap->build_flags_hash, sizeof(snap->build_flags_hash)) != 0) goto fail;
    { char tmp[64]; if (read_string(f, tmp, sizeof(tmp)) != 0) goto fail; }
    if (read_string(f, snap->manifest_config_hash, sizeof(snap->manifest_config_hash)) != 0) goto fail;
    if (read_string(f, snap->manifest_toolchain_hash, sizeof(snap->manifest_toolchain_hash)) != 0) goto fail;
    { char tmp[64]; if (read_string(f, tmp, sizeof(tmp)) != 0) goto fail; }
    if (read_string(f, snap->compiler_path, sizeof(snap->compiler_path)) != 0) goto fail;
    if (read_i64(f, &snap->compiler_mtime) != 0) goto fail;
    if (read_i64(f, &snap->compiler_size) != 0) goto fail;
    if (version >= SNAPSHOT_V2) {
        int32_t jobs, usize; uint8_t ub;
        if (read_i32(f, &jobs) != 0) goto fail;
        snap->jobs = jobs;
        if (fread(&ub, 1, 1, f) != 1) goto fail;
        snap->unity_build = ub;
        if (read_i32(f, &usize) != 0) goto fail;
        snap->unity_size = usize;
        if (read_string(f, snap->unity_disabled, sizeof(snap->unity_disabled)) != 0) goto fail;
    }
    if (read_i32(f, &snap->build_cs_count) != 0 || snap->build_cs_count < 0 || snap->build_cs_count > MAX_BUILDCS) goto fail;
    for (int i = 0; i < snap->build_cs_count; i++)
        if (read_string(f, snap->build_cs_paths[i], MAX_PATH_LEN) != 0) goto fail;
    if (read_i32(f, &snap->file_count) != 0 || snap->file_count < 0 || snap->file_count > MAX_FILES) goto fail;
    for (int i = 0; i < snap->file_count; i++) {
        if (read_string(f, snap->file_paths[i], MAX_PATH_LEN) != 0) goto fail;
        if (read_i64(f, &snap->file_sizes[i]) != 0) goto fail;
        if (read_i64(f, &snap->file_mtimes[i]) != 0) goto fail;
    }
    fclose(f);
    return 0;
fail:
    fclose(f);
    return -1;
}

#ifdef _WIN32
static int file_metadata(const char* path, int64_t* size, int64_t* mtime) {
    WIN32_FILE_ATTRIBUTE_DATA data;
    if (!GetFileAttributesExA(path, GetFileExInfoStandard, &data)) return -1;
    if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) return -1;
    *size = ((int64_t)data.nFileSizeHigh << 32) | data.nFileSizeLow;
    *mtime = ((int64_t)data.ftLastWriteTime.dwHighDateTime << 32) | data.ftLastWriteTime.dwLowDateTime;
    return 0;
}

static int count_build_cs(const char* source_root, int expected) {
    char pattern[MAX_PATH_LEN];
    int count = 0;
    snprintf(pattern, sizeof(pattern), "%s\\*", source_root);
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return -1;
    do {
        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0) continue;
        char child[MAX_PATH_LEN];
        snprintf(child, sizeof(child), "%s\\%s", source_root, fd.cFileName);
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            count += count_build_cs(child, expected);
            if (count > expected) break;
        } else {
            size_t len = strlen(fd.cFileName);
            if (len > 9 && _stricmp(fd.cFileName + len - 9, ".Build.cs") == 0) {
                count++;
                if (count > expected) break;
            }
        }
    } while (FindNextFileA(h, &fd));
    FindClose(h);
    return count;
}
#else
static int file_metadata(const char* path, int64_t* size, int64_t* mtime) {
    struct stat st;
    if (stat(path, &st) != 0 || !S_ISREG(st.st_mode)) return -1;
    *size = st.st_size;
    *mtime = (int64_t)st.st_mtime * 10000000LL + 116444736000000000LL;
    return 0;
}
#endif

static int validate_snapshot(const snapshot_t* snap, const char* engine_root) {
    int64_t size, mtime;
    if (snap->compiler_path[0]) {
        if (file_metadata(snap->compiler_path, &size, &mtime) != 0) return -1;
        if (size != snap->compiler_size || mtime != snap->compiler_mtime) return -1;
    }
    char source_root[MAX_PATH_LEN];
    snprintf(source_root, sizeof(source_root), "%s/Source", engine_root);
#ifdef _WIN32
    for (int i = 0; i < snap->build_cs_count; i++) {
        if (GetFileAttributesA(snap->build_cs_paths[i]) == INVALID_FILE_ATTRIBUTES) return -1;
    }
    int found = count_build_cs(source_root, snap->build_cs_count);
    if (found != snap->build_cs_count) return -1;
#else
    (void)source_root;
#endif
    for (int i = 0; i < snap->file_count; i++) {
        if (file_metadata(snap->file_paths[i], &size, &mtime) != 0) return -1;
        if (size != snap->file_sizes[i] || mtime != snap->file_mtimes[i]) return -1;
    }
    return 0;
}

static int matches_request(const snapshot_t* snap, const build_args_t* args) {
    char cfg[64];
    config_folder(args->config, cfg, sizeof(cfg));
    if (!ieq(snap->config, cfg)) return 0;
    if (!ieq(snap->platform, args->platform)) return 0;
    if (!ieq(snap->target, args->target)) return 0;
    if (snap->jobs != args->jobs) return 0;
    if (snap->unity_build != args->unity_build) return 0;
    if (snap->unity_size != args->unity_size) return 0;
    if (!ieq(snap->unity_disabled, args->unity_disabled)) return 0;
    if (!snap->manifest_config_hash[0] || !snap->manifest_toolchain_hash[0]) return 0;
    return 1;
}

int main(int argc, char** argv) {
#ifdef _WIN32
    LARGE_INTEGER freq, t0, t1;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&t0);
#else
    (void)freq;
#endif
    build_args_t args;
    snapshot_t snap;
    char snapshot_path[MAX_PATH_LEN];
    char manifest_path[MAX_PATH_LEN];

    if (parse_build_args(argc - 1, argv + 1, &args) != 0) return EXIT_BUILD;
    if (args.has_clean) return EXIT_BUILD;

#ifdef _WIN32
    snprintf(snapshot_path, sizeof(snapshot_path), "%s\\Build\\Database\\workspace_snapshot.bin", args.project_root);
    snprintf(manifest_path, sizeof(manifest_path), "%s\\Build\\Manifest\\build.manifest.bin", args.project_root);
#else
    snprintf(snapshot_path, sizeof(snapshot_path), "%s/Build/Database/workspace_snapshot.bin", args.project_root);
    snprintf(manifest_path, sizeof(manifest_path), "%s/Build/Manifest/build.manifest.bin", args.project_root);
#endif

    if (access(manifest_path, 0) != 0) return EXIT_BUILD;
    if (load_snapshot(snapshot_path, &snap) != 0) return EXIT_BUILD;
    if (!matches_request(&snap, &args)) return EXIT_BUILD;
    if (validate_snapshot(&snap, args.engine_root) != 0) return EXIT_BUILD;

#ifdef _WIN32
    QueryPerformanceCounter(&t1);
    double ms = (double)(t1.QuadPart - t0.QuadPart) * 1000.0 / (double)freq.QuadPart;
    printf("No-op build - nothing changed (%.0fms)\n", ms);
#else
    printf("No-op build - nothing changed\n");
#endif
    return EXIT_NOOP;
}
