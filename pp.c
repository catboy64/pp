#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <libgen.h>

#define MAX_PACKAGES 100 // TODO

// package info / pkg_list
typedef struct {
    char name[50];
    char version[20];
    char sha256[65];
    char url[256];
    int present_in_repository; // 0=not present, 1=exist, 2=to be update, 3=to be remove, 4=security update, 5=optional, 6= mandotary
} Package;

Package local_packages[MAX_PACKAGES];
int local_package_count = 0;

// find a package in the local_packages array by exact name
int find_local_package(const char *package_name) {
    for (int i = 0; i < local_package_count; i++) {
        if (strcmp(local_packages[i].name, package_name) == 0) {
            return i;
        }
    }
    return -1;
}

// write to the local_packages array to pp_pkg_list (excluding removed packages)
void write_local_package_list() {
    FILE *file = fopen("pp_pkg_list", "w");
    if (file == NULL) {
        perror("Error opening pp_pkg_list for writing");
        return;
    }

    for (int i = 0; i < local_package_count; i++) {
        if (local_packages[i].present_in_repository) {
            fprintf(file, "%s %s %s %s\n",
                    local_packages[i].name,
                    local_packages[i].version,
                    local_packages[i].sha256,
                    local_packages[i].url);
        }
    }

    fclose(file);
}

// read and parse pkg_list (repository source)
void read_repository_package_list() {
    FILE *file = fopen("pkg_list", "r"); // TODO: should be a url
    if (file == NULL) {
        perror("Error opening pkg_list");
        return;
    }

    char line[256]; // TODO
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;

        char *package_name = strtok(line, " ");
        char *version = strtok(NULL, " ");
        char *sha256 = strtok(NULL, " ");
        char *url = strtok(NULL, " ");
        // TODO: add array of flag

        if (package_name && version && sha256 && url) {
            int index = find_local_package(package_name);

            if (index != -1) {
                local_packages[index].present_in_repository = 1;

                if (strcmp(local_packages[index].version, version) != 0 ||
                    strcmp(local_packages[index].sha256, sha256) != 0 ||
                    strcmp(local_packages[index].url, url) != 0) {

                    printf("Updating package %s: Version %s -> %s, sha256 %s -> %s, URL %s -> %s\n",
                           package_name,
                           local_packages[index].version, version,
                           local_packages[index].sha256, sha256,
                           local_packages[index].url, url);

                    strncpy(local_packages[index].version, version, sizeof(local_packages[index].version) - 1);
                    local_packages[index].version[sizeof(local_packages[index].version) - 1] = '\0';
                    strncpy(local_packages[index].sha256, sha256, sizeof(local_packages[index].sha256) - 1);
                    local_packages[index].sha256[sizeof(local_packages[index].sha256) - 1] = '\0';
                    strncpy(local_packages[index].url, url, sizeof(local_packages[index].url) - 1);
                    local_packages[index].url[sizeof(local_packages[index].url) - 1] = '\0';
                } else {
                    printf("Package %s is up to date.\n", package_name);
                }
            } else {
                if (local_package_count < MAX_PACKAGES) {
                    printf("Adding new package to local list: %s\n", package_name);
                    strncpy(local_packages[local_package_count].name, package_name, sizeof(local_packages[local_package_count].name) - 1);
                    local_packages[local_package_count].name[sizeof(local_packages[local_package_count].name) - 1] = '\0';
                    strncpy(local_packages[local_package_count].version, version, sizeof(local_packages[local_package_count].version) - 1);
                    local_packages[local_package_count].version[sizeof(local_packages[local_package_count].version) - 1] = '\0';
                    strncpy(local_packages[local_package_count].sha256, sha256, sizeof(local_packages[local_package_count].sha256) - 1);
                    local_packages[local_package_count].sha256[sizeof(local_packages[local_package_count].sha256) - 1] = '\0';
                    strncpy(local_packages[local_package_count].url, url, sizeof(local_packages[local_package_count].url) - 1);
                    local_packages[local_package_count].url[sizeof(local_packages[local_package_count].url) - 1] = '\0';
                    local_packages[local_package_count].present_in_repository = 1;
                    local_package_count++;
                } else {
                    printf("Warning: Maximum package limit reached. Cannot add %s\n", package_name);
                }
            }
        } else {
            printf("Skipping invalid line in pkg_list: %s\n", line);
        }
    }
    fclose(file);
}

// read and parse pp_pkg_list(local package list)
void read_local_package_list() {
    FILE *file = fopen("pp_pkg_list", "r");
    if (file == NULL) {
        for (int i = 0; i < MAX_PACKAGES; i++) {
            local_packages[i].present_in_repository = 0;
        }
        return;
    }

    char line[256]; // TODO: can be longer
    while (fgets(line, sizeof(line), file) && local_package_count < MAX_PACKAGES) {
        line[strcspn(line, "\n")] = 0;

        char *package_name = strtok(line, " ");
        char *version = strtok(NULL, " ");
        char *sha256 = strtok(NULL, " ");
        char *url = strtok(NULL, " ");

        if (package_name && version && sha256 && url) {
            strncpy(local_packages[local_package_count].name, package_name, sizeof(local_packages[local_package_count].name) - 1);
            local_packages[local_package_count].name[sizeof(local_packages[local_package_count].name) - 1] = '\0';
            strncpy(local_packages[local_package_count].version, version, sizeof(local_packages[local_package_count].version) - 1);
            local_packages[local_package_count].version[sizeof(local_packages[local_package_count].version) - 1] = '\0';
            strncpy(local_packages[local_package_count].sha256, sha256, sizeof(local_packages[local_package_count].sha256) - 1);
            local_packages[local_package_count].sha256[sizeof(local_packages[local_package_count].sha256) - 1] = '\0';
            strncpy(local_packages[local_package_count].url, url, sizeof(local_packages[local_package_count].url) - 1);
            local_packages[local_package_count].url[sizeof(local_packages[local_package_count].url) - 1] = '\0';
            local_packages[local_package_count].present_in_repository = 0;

            local_package_count++;
        } else {
            printf("Skipping invalid line in pp_pkg_list: %s\n", line);
        }
    }

    fclose(file);
}

// search for a package
void search_package(const char *search_term) {
    printf("Searching for packages matching: %s\n", search_term);

    int found_count = 0;
    for (int i = 0; i < local_package_count; i++) {
        if (strstr(local_packages[i].name, search_term) != NULL) {
            printf("Found package: Name=%s, Version=%s, sha256=%s, URL=%s\n",
                   local_packages[i].name,
                   local_packages[i].version,
                   local_packages[i].sha256,
                   local_packages[i].url);
            found_count++;
        }
    }

    if (found_count == 0) {
        printf("No packages found matching '%s'.\n", search_term);
    }
}

// find and print information for a package by exact name
void exact_search_package(const char *package_name_to_find) {
    printf("Searching for exact package match: %s\n", package_name_to_find);

    int found_index = find_local_package(package_name_to_find);

    if (found_index != -1) {
        printf("Found package: Name=%s, Version=%s, sha256=%s, URL=%s\n",
               local_packages[found_index].name,
               local_packages[found_index].version,
               local_packages[found_index].sha256,
               local_packages[found_index].url);
    } else {
        printf("Package '%s' not found in local package list.\n", package_name_to_find);
    }
}

// install a package
void install_package(const char *package_name) {
    printf("Attempting to install package: %s\n", package_name);

    read_local_package_list(); // local package list is loaded
    int package_index = find_local_package(package_name);

    if (package_index == -1) {
        printf("Error: Package '%s' not found in local package list. Cannot install.\n", package_name);
        return;
    }

    const char *package_url = local_packages[package_index].url;
    const char *package_sha256 = local_packages[package_index].sha256;
    printf("Package URL: %s\n", package_url);

    char confirm_install[10];
    printf("Install %s? (Y/n): ", package_name);
    if (fgets(confirm_install, sizeof(confirm_install), stdin) != NULL) {
        confirm_install[strcspn(confirm_install, "\n")] = 0;

        if (strlen(confirm_install) == 0 ||
            strcmp(confirm_install, "Y") == 0 || strcmp(confirm_install, "y") == 0) {

            printf("Installing %s...\n", package_name);

            printf("Creating pp_download directory...\n");
            if (mkdir("pp_download", 0755) == -1) {
                if (errno != EEXIST) {
                    perror("Error creating pp_download directory");
                    return;
                }
            }

            // download the package file to pp_download
            char download_path[512];
            char url_copy[256]; 
            strncpy(url_copy, package_url, sizeof(url_copy) - 1);
            url_copy[sizeof(url_copy) - 1] = '\0';
            char *filename = basename(url_copy);

            snprintf(download_path, sizeof(download_path), "pp_download/%s", filename);
            printf("Destination path: %s\n", download_path);


            if (strncmp(package_url, "http://", 7) == 0 || strncmp(package_url, "https://", 8) == 0) { // url
                printf("Downloading package from %s...\n", package_url);
                char curl_command[1024];
                snprintf(curl_command, sizeof(curl_command), "curl -L \"%s\" -o \"%s\"", package_url, download_path);
                printf("Executing command: %s\n", curl_command);
                int curl_status = system(curl_command);
                if (curl_status != 0) {
                    printf("Error downloading package: curl command failed with status %d\n", curl_status);
                    return;
                }
                 printf("Download complete.\n");
            } else { // local file path
                printf("Copying package from local path %s...\n", package_url);
                FILE *source_file = fopen(package_url, "rb");
                if (source_file == NULL) {
                    perror("Error opening local package file");
                    return;
                }

                FILE *dest_file = fopen(download_path, "wb");
                if (dest_file == NULL) {
                    perror("Error creating destination file in pp_download");
                    fclose(source_file);
                    return;
                }

                char buffer[4096];
                size_t bytes_read;
                while ((bytes_read = fread(buffer, 1, sizeof(buffer), source_file)) > 0) {
                    fwrite(buffer, 1, bytes_read, dest_file);
                }

                fclose(source_file);
                fclose(dest_file);
                printf("Copy complete.\n");
            }

            // untar the file into pp_download
            char untar_dir[512];
            snprintf(untar_dir, sizeof(untar_dir), "pp_download/%s", package_name);
            printf("Creating untar directory: %s\n", untar_dir);
             if (mkdir(untar_dir, 0755) == -1) {
                if (errno != EEXIST) { // directory already exist
                    perror("Error creating untar directory");
                    return;
                }
            }

            char tar_command[1024];
            snprintf(tar_command, sizeof(tar_command), "tar -xf \"%s\" -C \"%s\"", download_path, untar_dir);
            printf("Executing command: %s\n", tar_command);
            int tar_status = system(tar_command);
             if (tar_status != 0) {
                printf("Error untarring package: tar command failed with status %d\n", tar_status);
                return;
            }
            printf("Untar complete.\n");

            // find and read the MANIFEST
            char manifest_path[512];
            snprintf(manifest_path, sizeof(manifest_path), "%s/MANIFEST", untar_dir);
            printf("Looking for MANIFEST file at: %s\n", manifest_path);

            FILE *manifest_file = fopen(manifest_path, "r");
            char full_manifest_content[4096] = ""; // Assuming MANIFEST is not larger than 4KB
            if (manifest_file == NULL) {
                perror("Error opening MANIFEST file");
                printf("MANIFEST file not found at %s. Skipping manifest-related steps.\n", manifest_path);
            } else {
                printf("--- MANIFEST ---\n");
                char manifest_line[256];
                while (fgets(manifest_line, sizeof(manifest_line), manifest_file)) {
                    printf("%s", manifest_line);
                    strncat(full_manifest_content, manifest_line, sizeof(full_manifest_content) - strlen(full_manifest_content) - 1);
                }
                printf("----------------\n");
                fclose(manifest_file);

                // save MANIFEST and uninstall script to pp_info/PACKAGENAME/. -> fake database
                char pp_info_dir[512];
                snprintf(pp_info_dir, sizeof(pp_info_dir), "pp_info/%s", package_name);
                printf("Creating package info directory: %s\n", pp_info_dir);
                if (mkdir("pp_info", 0755) == -1) {
                     if (errno != EEXIST) {
                        perror("Error creating pp_info directory");
                        return; // todo?
                    }
                }
                if (mkdir(pp_info_dir, 0755) == -1) {
                     if (errno != EEXIST) {
                        perror("Error creating package info directory");
                        return;
                    }
                } else {
                     // save MANIFEST
                    char saved_manifest_path[512];
                    snprintf(saved_manifest_path, sizeof(saved_manifest_path), "%s/MANIFEST", pp_info_dir);
                    printf("Saving MANIFEST to: %s\n", saved_manifest_path);
                    FILE *saved_manifest_file = fopen(saved_manifest_path, "w");
                    if (saved_manifest_file == NULL) {
                        perror("Error saving MANIFEST file");
                    } else {
                        fprintf(saved_manifest_file, "%s", full_manifest_content);
                        fclose(saved_manifest_file);
                    }

                    // parse MANIFEST
                    char *uninstall_script_line = strstr(full_manifest_content, "uninstall:");
                    if (uninstall_script_line != NULL) {
                        char *uninstall_script_name = uninstall_script_line + strlen("uninstall:");
                         // leading whitespace
                        while (*uninstall_script_name == ' ' || *uninstall_script_name == '\t') {
                            uninstall_script_name++;
                        }
                        // end of script name
                         char *end = uninstall_script_name;
                         while (*end != '\n' && *end != '#' && *end != '\0') {
                             end++;
                         }
                         *end = '\0';

                        if (strlen(uninstall_script_name) > 0) {
                            char source_uninstall_script_path[512];
                            snprintf(source_uninstall_script_path, sizeof(source_uninstall_script_path), "%s/%s", untar_dir, uninstall_script_name);

                            char dest_uninstall_script_path[512];
                            snprintf(dest_uninstall_script_path, sizeof(dest_uninstall_script_path), "%s/%s", pp_info_dir, uninstall_script_name);

                            printf("Looking for uninstall script at: %s\n", source_uninstall_script_path);
                            FILE *source_uninstall_script = fopen(source_uninstall_script_path, "rb");
                            if (source_uninstall_script == NULL) {
                                perror("Error opening uninstall script");
                                printf("Uninstall script '%s' not found in package.\n", uninstall_script_name);
                            } else {
                                 printf("Saving uninstall script to: %s\n", dest_uninstall_script_path);
                                FILE *dest_uninstall_script = fopen(dest_uninstall_script_path, "wb");
                                if (dest_uninstall_script == NULL) {
                                    perror("Error saving uninstall script");
                                    fclose(source_uninstall_script);
                                } else {
                                     char script_buffer[4096];
                                    size_t script_bytes_read;
                                     while ((script_bytes_read = fread(script_buffer, 1, sizeof(script_buffer), source_uninstall_script)) > 0) {
                                        fwrite(script_buffer, 1, script_bytes_read, dest_uninstall_script);
                                    }
                                    fclose(source_uninstall_script);
                                    fclose(dest_uninstall_script);
                                    printf("Uninstall script saved to pp_info.\n");
                                }
                            }
                        } else {
                             printf("Uninstall script specified in MANIFEST is empty.\n");
                        }
                    } else {
                         printf("No uninstall script specified in MANIFEST.\n");
                    }
                }

                char *install_script_line = strstr(full_manifest_content, "install:");
                if (install_script_line != NULL) {
                    char *install_script_name = install_script_line + strlen("install:");
                    // trim whitespace
                    while (*install_script_name == ' ' || *install_script_name == '\t') {
                        install_script_name++;
                    }
                    // end of script name
                    char *end = install_script_name;
                    while (*end != '\n' && *end != '#' && *end != '\0') {
                        end++;
                    }
                    *end = '\0';

                    if (strlen(install_script_name) > 0) {
                        char install_script_relative_path[512];
                        snprintf(install_script_relative_path, sizeof(install_script_relative_path), "%s/%s", untar_dir, install_script_name);

                        printf("Looking for install script at: %s\n", install_script_relative_path);

                        char full_install_script_path[PATH_MAX];
                        if (realpath(install_script_relative_path, full_install_script_path) == NULL) {
                            perror("Error getting full path for install script");
                            printf("Could not get full path for install script '%s'. Cannot execute.\n", install_script_relative_path);
                        } else {
                             printf("Full install script path: %s\n", full_install_script_path);
                            if (chmod(full_install_script_path, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == 0) {
                                 printf("Made install script executable.\n");
                                 printf("Executing install script: %s\n", full_install_script_path);
                                 int script_status = system(full_install_script_path);
                                if (script_status != 0) {
                                    printf("Error executing install script: script failed with status %d\n", script_status);
                                } else {
                                    printf("Install script execution complete.\n");
                                }
                            } else {
                                 perror("Error making install script executable");
                                printf("Could not make install script '%s' executable.\n", full_install_script_path);
                            }
                        }
                    } else {
                        printf("Install script specified in MANIFEST is empty.\n");
                    }
                } else {
                    printf("No install script specified in MANIFEST.\n");
                }
            }

            // TODO: clean up downloaded and untarred files in pp_download after

        } else if (strcmp(confirm_install, "N") == 0 || strcmp(confirm_install, "n") == 0) {
            printf("Skipping installation for %s.\n", package_name);
        } else {
            printf("Invalid input. Skipping installation for %s.\n", package_name);
        }
    } else {
        printf("Error reading confirmation input. Skipping installation for %s.\n", package_name);
    }
}


// remove a package
void remove_package(const char *package_name) {
    printf("Attempting to remove package: %s\n", package_name);

    char pp_info_dir[512];
    snprintf(pp_info_dir, sizeof(pp_info_dir), "pp_info/%s", package_name);

    struct stat st;
    if (stat(pp_info_dir, &st) == -1) {
        if (errno == ENOENT) {
            printf("Package '%s' not found in pp_info. It might not be installed.\n", package_name);
        } else {
            perror("Error checking package info directory");
        }
        return;
    }

    char confirm_remove[10];
    printf("Remove %s? (Y/n): ", package_name);
    if (fgets(confirm_remove, sizeof(confirm_remove), stdin) != NULL) {
        confirm_remove[strcspn(confirm_remove, "\n")] = 0;

        if (strlen(confirm_remove) == 0 ||
            strcmp(confirm_remove, "Y") == 0 || strcmp(confirm_remove, "y") == 0) {

            printf("Removing %s...\n", package_name);

            char manifest_path[512];
            snprintf(manifest_path, sizeof(manifest_path), "%s/MANIFEST", pp_info_dir);

            char full_manifest_content[4096] = ""; // Assuming MANIFEST is not larger than 4KB
            FILE *manifest_file = fopen(manifest_path, "r");
            if (manifest_file == NULL) {
                perror("Error opening MANIFEST file in pp_info");
                printf("MANIFEST file not found for package '%s' in pp_info. Cannot execute uninstall script.\n", package_name);
            } else {
                char manifest_line[256];
                while (fgets(manifest_line, sizeof(manifest_line), manifest_file)) {
                    strncat(full_manifest_content, manifest_line, sizeof(full_manifest_content) - strlen(full_manifest_content) - 1);
                }
                fclose(manifest_file);

                // parse MANIFEST
                char *uninstall_script_line = strstr(full_manifest_content, "uninstall:");
                char uninstall_script_name[256] = "";
                char uninstall_script_path[512] = "";

                if (uninstall_script_line != NULL) {
                    char *temp_script_name = uninstall_script_line + strlen("uninstall:");
                    // trim whitespace
                    while (*temp_script_name == ' ' || *temp_script_name == '\t') {
                        temp_script_name++;
                    }
                    // end of script name
                    char *end = temp_script_name;
                    while (*end != '\n' && *end != '#' && *end != '\0') {
                        end++;
                    }
                    size_t name_len = end - temp_script_name;
                    if (name_len > 0) {
                        strncpy(uninstall_script_name, temp_script_name, sizeof(uninstall_script_name) - 1);
                        uninstall_script_name[sizeof(uninstall_script_name) - 1] = '\0';
                        snprintf(uninstall_script_path, sizeof(uninstall_script_path), "%s/%s", pp_info_dir, uninstall_script_name);

                        printf("Looking for uninstall script at: %s\n", uninstall_script_path);

                        char full_uninstall_script_path[PATH_MAX];
                        if (realpath(uninstall_script_path, full_uninstall_script_path) == NULL) {
                             perror("Error getting full path for uninstall script");
                            printf("Could not get full path for uninstall script '%s'. Cannot execute.\n", uninstall_script_path);
                        } else {
                             printf("Full uninstall script path: %s\n", full_uninstall_script_path);

                             if (chmod(full_uninstall_script_path, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == 0) {
                                 printf("Made uninstall script executable.\n");
                                 printf("Executing uninstall script: %s\n", full_uninstall_script_path);
                                 int script_status = system(full_uninstall_script_path);
                                 if (script_status != 0) {
                                     printf("Error executing uninstall script: script failed with status %d\n", script_status);
                                 } else {
                                     printf("Uninstall script execution complete.\n");
                                 }
                             } else {
                                  perror("Error making uninstall script executable");
                                 printf("Could not make uninstall script '%s' executable.\n", full_uninstall_script_path);
                             }
                        }
                    } else {
                        printf("Uninstall script specified in MANIFEST is empty.\n");
                    }
                } else {
                    printf("No uninstall script specified in MANIFEST.\n");
                }

                // remove the MANIFEST file and the uninstall script (if it exists).
                char saved_manifest_path[512];
                snprintf(saved_manifest_path, sizeof(saved_manifest_path), "%s/MANIFEST", pp_info_dir);
                printf("Removing MANIFEST file: %s\n", saved_manifest_path);
                if (remove(saved_manifest_path) != 0) {
                    perror("Error removing MANIFEST file");
                } else {
                    printf("MANIFEST file removed.\n");
                }

                if (strlen(uninstall_script_name) > 0) {
                    char uninstall_script_full_path_to_remove[PATH_MAX];
                     if (realpath(uninstall_script_path, uninstall_script_full_path_to_remove) == NULL) {
                         perror("Error getting full path for uninstall script to remove");
                        printf("Could not get full path for uninstall script '%s'. May require manual removal.\n", uninstall_script_path);
                     } else {
                         printf("Removing uninstall script: %s\n", uninstall_script_full_path_to_remove);
                         if (remove(uninstall_script_full_path_to_remove) != 0) {
                             perror("Error removing uninstall script");
                         } else {
                             printf("Uninstall script removed.\n");
                         }
                     }
                }
            }

            // remove the pp_info/PACKAGENAME/ directory
            printf("Removing package info directory: %s\n", pp_info_dir);
            if (rmdir(pp_info_dir) == -1) {
                 perror("Error removing package info directory");
                 printf("Directory might not be empty after uninstall. You might need to manually remove the directory: %s\n", pp_info_dir);
            } else {
                printf("Package info directory removed.\n");
            }

        } else if (strcmp(confirm_remove, "N") == 0 || strcmp(confirm_remove, "n") == 0) {
            printf("Skipping removal for %s.\n", package_name);
        } else {
            printf("Invalid input. Skipping removal for %s.\n", package_name);
        }
    } else {
        printf("Error reading confirmation input. Skipping removal for %s.\n", package_name);
    }
}


// upgrade packages that are installed locally (present in pp_info) but have a newer version available.
void upgrade_packages() {
    printf("Checking for upgrades...\n");

    // TODO: lu command for this
    printf("Updating local system metadata...\n");
    read_local_package_list();
    read_repository_package_list();
    write_local_package_list();
    printf("Local system metadata updated.\n");

    printf("Identifying upgradable packages...\n");
    for (int i = 0; i < local_package_count; i++) {
        // check if the package is installed
        char pp_info_dir[512];
        snprintf(pp_info_dir, sizeof(pp_info_dir), "pp_info/%s", local_packages[i].name);

        struct stat st;
        if (stat(pp_info_dir, &st) == 0 && S_ISDIR(st.st_mode)) {
            // Package is installed, now check for a newer version

            // read the version from the saved MANIFEST in pp_info
            char manifest_path_in_info[512];
            snprintf(manifest_path_in_info, sizeof(manifest_path_in_info), "%s/MANIFEST", pp_info_dir);

            FILE *manifest_file_in_info = fopen(manifest_path_in_info, "r");
            char installed_version[20] = ""; // Buffer for installed version

            if (manifest_file_in_info != NULL) { // TODO: function to read the manifest maybe?
                char manifest_line[256];
                while (fgets(manifest_line, sizeof(manifest_line), manifest_file_in_info)) {
                    if (strstr(manifest_line, "version:") != NULL) {
                        char *version_str = strstr(manifest_line, "version:") + strlen("version:");
                        // Trim leading whitespace
                        while (*version_str == ' ' || *version_str == '\t') {
                            version_str++;
                        }
                        // end of version string
                        char *end = version_str;
                        while (*end != '\n' && *end != '#' && *end != '\0') {
                            end++;
                        }
                        size_t version_len = end - version_str;
                        if (version_len > 0) {
                            strncpy(installed_version, version_str, sizeof(installed_version) - 1);
                            installed_version[sizeof(installed_version) - 1] = '\0';
                        }
                        break;
                    }
                }
                fclose(manifest_file_in_info);
            }

            // TODO: compare versions
            if (strlen(installed_version) > 0 && strcmp(local_packages[i].version, installed_version) > 0) {
                printf("Upgrade available for %s (Installed: %s, Available: %s)\n", 
                        local_packages[i].name, 
                        installed_version, 
                        local_packages[i].version);
                char confirm_upgrade[10];
                printf("Upgrade %s? (Y/n): ", local_packages[i].name);
                if (fgets(confirm_upgrade, sizeof(confirm_upgrade), stdin) != NULL) {
                    confirm_upgrade[strcspn(confirm_upgrade, "\n")] = 0;

                    if (strlen(confirm_upgrade) == 0 ||
                        strcmp(confirm_upgrade, "Y") == 0 || strcmp(confirm_upgrade, "y") == 0) {

                        printf("Upgrading %s...\n", local_packages[i].name);

                        // uninstall the old version and install the new one.
                        char full_manifest_content_in_info[4096] = "";
                        char uninstall_script_name[256] = "";

                        manifest_file_in_info = fopen(manifest_path_in_info, "r");

                        if (manifest_file_in_info != NULL) {
                            char manifest_line_in_info[256];
                            while (fgets(manifest_line_in_info, sizeof(manifest_line_in_info), manifest_file_in_info)) {
                                strncat(full_manifest_content_in_info, manifest_line_in_info, sizeof(full_manifest_content_in_info) - strlen(full_manifest_content_in_info) - 1);
                            }
                            fclose(manifest_file_in_info);

                            char *uninstall_script_line = strstr(full_manifest_content_in_info, "uninstall:");
                            if (uninstall_script_line != NULL) {
                                char *temp_script_name = uninstall_script_line + strlen("uninstall:");

                                // trim whitespace
                                while (*temp_script_name == ' ' || *temp_script_name == '\t') {
                                    temp_script_name++;
                                }

                                // end of script name
                                char *end = temp_script_name;
                                while (*end != '\n' && *end != '#' && *end != '\0') {
                                    end++;
                                }
                                size_t name_len = end - temp_script_name;
                                if (name_len > 0) {
                                     strncpy(uninstall_script_name, temp_script_name, sizeof(uninstall_script_name) - 1);
                                     uninstall_script_name[sizeof(uninstall_script_name) - 1] = '\0';
                                }
                            }
                        } else {
                            perror("Error reading MANIFEST for old version uninstall script");
                        }


                        if (strlen(uninstall_script_name) > 0) {
                             char uninstall_script_relative_path[512];
                            snprintf(uninstall_script_relative_path, sizeof(uninstall_script_relative_path), "%s/%s", pp_info_dir, uninstall_script_name);
                            printf("Executing uninstall script for old version: %s\n", uninstall_script_relative_path);

                            char full_uninstall_script_path[PATH_MAX];
                            if (realpath(uninstall_script_relative_path, full_uninstall_script_path) == NULL) {
                                 perror("Error getting full path for uninstall script");
                                printf("Could not get full path for uninstall script '%s'. Skipping uninstall.\n", uninstall_script_relative_path);
                            } else {
                                 printf("Full uninstall script path: %s\n", full_uninstall_script_path);
                                if (chmod(full_uninstall_script_path, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == 0) {
                                     printf("Made uninstall script executable.\n");
                                    int script_status = system(full_uninstall_script_path);
                                    if (script_status != 0) {
                                        printf("Error executing uninstall script for old version: script failed with status %d\n", script_status);
                                    } else {
                                        printf("Uninstall script for old version executed successfully.\n");
                                    }
                                } else {
                                     perror("Error making uninstall script executable");
                                    printf("Could not make uninstall script '%s' executable. Skipping uninstall.\n", full_uninstall_script_path);
                                }
                            }
                        } else {
                            printf("No uninstall script specified in MANIFEST for old version.\n");
                        }

                        // elements to remove, manifest and uninstall script, directory
                        printf("Removing package info directory for old version: %s\n", pp_info_dir);
                        char old_manifest_path[512];
                        snprintf(old_manifest_path, sizeof(old_manifest_path), "%s/MANIFEST", pp_info_dir);
                        printf("Removing old MANIFEST file: %s\n", old_manifest_path);
                        if (remove(old_manifest_path) != 0) {
                            perror("Error removing old MANIFEST file");
                        } else {
                            printf("Old MANIFEST file removed.\n");
                        }

                        if (strlen(uninstall_script_name) > 0) {
                            char old_uninstall_script_path[512];
                             snprintf(old_uninstall_script_path, sizeof(old_uninstall_script_path), "%s/%s", pp_info_dir, uninstall_script_name);
                             printf("Removing old uninstall script: %s\n", old_uninstall_script_path);
                             if (remove(old_uninstall_script_path) != 0) {
                                 perror("Error removing old uninstall script");
                             } else {
                                 printf("Old uninstall script removed.\n");
                             }
                        }

                        if (rmdir(pp_info_dir) == -1) {
                            perror("Error removing package info directory for old version");
                            printf("Directory might not be empty after uninstall. You might need to manually remove the directory: %s\n", pp_info_dir);
                        } else {
                            printf("Package info directory for old version removed.\n");
                        }
                        printf("Installing new version of %s...\n", local_packages[i].name);
                        install_package(local_packages[i].name);
                    } else if (strcmp(confirm_upgrade, "N") == 0 || strcmp(confirm_upgrade, "n") == 0) {
                        printf("Skipping upgrade for %s.\n", local_packages[i].name);
                    } else {
                        printf("Invalid input. Skipping upgrade for %s.\n", local_packages[i].name);
                    }
                } else {
                    printf("Error reading confirmation input. Skipping upgrade for %s.\n", local_packages[i].name);
                }
            } else {
                printf("Package %s is installed and up to date (Version: %s).\n",
                       local_packages[i].name, local_packages[i].version);
            }
        } else {
            // TODO: package is not installed locally, check if it is available
        }
    }
    printf("Upgrade check complete.\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: pp [command] [package_name]\n");
        return 1;
    }

    char *command = argv[1];
    char *package_name = NULL;

    if (argc > 2) {
        package_name = argv[2];
    }

    printf("Command: %s\n", command);
    if (package_name) {
        printf("Package Name: %s\n", package_name);
    }

    if (strcmp(command, "lu") == 0) {
        printf("Updating local system metadata...\n");
        read_local_package_list();
        read_repository_package_list();
        write_local_package_list(); // pp_pkg_list
    } else if (strcmp(command, "s") == 0) {
        if (package_name == NULL) {
            printf("Usage: pp s [search_term]\n");
            return 1;
        }
        read_local_package_list();
        search_package(package_name);
    } else if (strcmp(command, "e") == 0) {
        if (package_name == NULL) {
            printf("Usage: pp e [package_name]\n");
            return 1;
        }
        read_local_package_list();
        exact_search_package(package_name);
    } else if (strcmp(command, "i") == 0) {
         if (package_name == NULL) {
            printf("Usage: pp i [package_name]\n");
            return 1;
        }
        install_package(package_name);
    } else if (strcmp(command, "r") == 0) {
         if (package_name == NULL) {
            printf("Usage: pp r [package_name]\n");
            return 1;
        }
        remove_package(package_name);
    } else if (strcmp(command, "up") == 0) {
        upgrade_packages();
    }
    else {
        printf("Unknown command: %s\n", command);
        printf("Usage: pp [command] [package_name]\n");
        printf("Usage: pp [i|r|s|e] PACKAGENAME | pp [up|lu]\n");
        return 1;
    }
    return 0;
}