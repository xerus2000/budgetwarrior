//=======================================================================
// Copyright (c) 2013-2017 Baptiste Wicht.
// Distributed under the terms of the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include <iostream>
#include <fstream>
#include <unordered_map>

#include <unistd.h>    //for getuid
#include <sys/types.h> //for getuid
#include <sys/stat.h>  //For mkdir
#ifdef _WIN32
#include <shlobj.h>
#else
#include <pwd.h>       //For getpwuid
#endif

#include "config.hpp"
#include "utils.hpp"

using namespace budget;

typedef std::unordered_map<std::string, std::string> config_type;

namespace {

bool load_configuration(const std::string& path, config_type& configuration){
    if (file_exists(path)) {
        std::ifstream file(path);

        if (file.is_open() && file.good()) {
            std::string line;
            while (file.good() && getline(file, line)) {
                if (line.empty()) {
                    continue;
                }

                auto first = line.find('=');

                if (first == std::string::npos || line.rfind('=') != first) {
                    std::cout << "The configuration file " << path << " is invalid only supports entries in form of key=value" << std::endl;

                    return false;
                }

                auto key   = line.substr(0, first);
                auto value = line.substr(first + 1, line.size());

                configuration[key] = value;
            }
        }
    }

    return true;
}

void save_configuration(const std::string& path, const config_type& configuration){
    std::ofstream file(path);

    for(auto& entry : configuration){
        file << entry.first << "=" << entry.second << std::endl;
    }
}

bool verify_folder(){
    auto folder_path = budget_folder();

    if(!folder_exists(folder_path)){
        std::cout << "The folder " << folder_path << " does not exist. Would like to create it [yes/no] ? ";

        std::string answer;
        std::cin >> answer;

        if(answer == "yes" || answer == "y"){
#ifdef _WIN32
            if(mkdir(folder_path.c_str()) == 0){
#else
            if(mkdir(folder_path.c_str(), ACCESSPERMS) == 0){
#endif
                std::cout << "The folder " << folder_path << " was created. " << std::endl;

                return true;
            } else {
                std::cout << "Impossible to create the folder " << folder_path << std::endl;

                return false;
            }
        } else {
            return false;
        }
    }

    return true;
}

} //end of anonymous namespace

static config_type configuration;
static config_type internal;
static config_type internal_bak;

bool budget::load_config(){
    if(!load_configuration(path_to_home_file(".budgetrc"), configuration)){
        return false;
    }

    if(!verify_folder()){
        return false;
    }

    if(!load_configuration(path_to_budget_file("config"), internal)){
        return false;
    }

    internal_bak = internal;

    //At the first start, the version is not set
    if(internal.find("data_version") == internal.end()){
        internal["data_version"] = budget::to_string(budget::DATA_VERSION);
    }

    return true;
}

void budget::save_config(){
    if(internal != internal_bak){
        save_configuration(path_to_budget_file("config"), internal);
    }
}

std::string budget::home_folder(){
#ifdef _WIN32
    TCHAR path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PROFILE, NULL, 0, path))) {
        std::wstring wpath(path);
        return std::string(wpath.begin(), wpath.end());
    }
    return "c:\\";
#else
    struct passwd *pw = getpwuid(getuid());

    const char* homedir = pw->pw_dir;

    return std::string(homedir);
#endif
}

std::string budget::path_to_home_file(const std::string& file){
    return home_folder() + "/" + file;
}

std::string budget::budget_folder(){
    if(config_contains("directory")){
        return config_value("directory");
    }

    return path_to_home_file(".budget");
}

std::string budget::path_to_budget_file(const std::string& file){
    return budget_folder() + "/" + file;
}

bool budget::config_contains(const std::string& key){
    return configuration.find(key) != configuration.end();
}

std::string budget::config_value(const std::string& key){
    return configuration[key];
}

bool budget::internal_config_contains(const std::string& key){
    return internal.find(key) != internal.end();
}

std::string& budget::internal_config_value(const std::string& key){
    return internal[key];
}

void budget::internal_config_remove(const std::string& key){
    internal.erase(key);
}

std::string budget::get_web_user(){
    if (config_contains("web_user")) {
        return config_value("web_user");
    }

    return "admin";
}

std::string budget::get_web_password(){
    if (config_contains("web_password")) {
        return config_value("web_password");
    }

    return "1234";
}
