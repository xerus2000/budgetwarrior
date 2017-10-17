//=======================================================================
// Copyright (c) 2013-2017 Baptiste Wicht.
// Distributed under the terms of the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#ifndef DATA_H
#define DATA_H

#include "cpp_utils/assert.hpp"

#include "config.hpp"
#include "utils.hpp"

namespace budget {

template<typename T>
struct data_handler {
    size_t next_id;
    std::vector<T> data;
    bool changed = false;

    data_handler(){};

    //data_handler should never be copied
    data_handler(const data_handler& rhs) = delete;
    data_handler& operator=(const data_handler& rhs) = delete;
};

template<typename T>
void save_data(const data_handler<T>& data, const std::string& path){
    if (data.changed) {
        if (budget::config_contains("random")) {
            std::cerr << "budget: error: Saving is disabled in random mode" << std::endl;
            return;
        }

        auto file_path = path_to_budget_file(path);

        std::ofstream file(file_path);

        // We still save the file ID so that it's still compatible with older versions for now
        file << data.next_id << std::endl;

        for(auto& entry: data.data){
            file << entry << std::endl;
        }
    }
}

template<typename T, typename Functor>
void load_data(data_handler<T>& data, const std::string& path, Functor f){
    auto file_path = path_to_budget_file(path);

    if(!file_exists(file_path)){
        data.next_id = 1;
    } else {
        std::ifstream file(file_path);

        if(file.is_open()){
            if(file.good()){
                //Make sure to clear the data first, as load_data can be called
                //several times
                data.data.clear();

                // We do not use the next_id saved anymore
                size_t fake;
                file >> fake;
                file.get();

                data.next_id = 1;

                std::string line;
                while(file.good() && getline(file, line)){
                    auto parts = split(line, ':');

                    T entry;

                    f(parts, entry);

                    if(entry.id >= data.next_id){
                        data.next_id = entry.id + 1;
                    }

                    data.data.push_back(std::move(entry));
                }
            }
        }
    }
}

template<typename T>
void load_data(data_handler<T>& data, const std::string& path){
    load_data(data, path, [](std::vector<std::string>& parts, T& entry){ parts >> entry; });
}

template<typename T>
bool exists(const data_handler<T>& data, size_t id){
    for(auto& entry : data.data){
        if(entry.id == id){
            return true;
        }
    }

    return false;
}

template<typename T>
void remove(data_handler<T>& data, size_t id){
    data.data.erase(std::remove_if(data.data.begin(), data.data.end(),
        [id](const T& entry){ return entry.id == id; }), data.data.end());

    data.changed = true;
}

template<typename T>
T& get(data_handler<T>& data, size_t id){
    for(auto& value : data.data){
        if(value.id == id){
            return value;
        }
    }

    cpp_unreachable("The data must exists");
}

template<typename T>
size_t add_data(data_handler<T>& data, T&& entry){
    entry.id = data.next_id++;

    data.data.push_back(std::forward<T>(entry));

    data.changed = true;

    return entry.id;
}

} //end of namespace budget

#endif
