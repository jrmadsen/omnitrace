// MIT License
//
// Copyright (c) 2022 Advanced Micro Devices, Inc. All Rights Reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "config_groups.hpp"
#include "common.hpp"
#include "config.hpp"
#include "debug.hpp"
#include "timemory/utility/delimit.hpp"

#include <timemory/settings/settings.hpp>

#include <toml++/impl/forward_declarations.hpp>
#include <toml++/toml.hpp>

#include <memory>

#define OMNITRACE_CONFIG_SETTING(TYPE, ENV_NAME, DESCRIPTION, INITIAL_VALUE, CALLBACK,   \
                                 ...)                                                    \
    [&]() {                                                                              \
        auto _ret = _config->insert<TYPE, TYPE>(                                         \
            ENV_NAME, get_setting_name(ENV_NAME), DESCRIPTION, TYPE{ INITIAL_VALUE },    \
            CALLBACK,                                                                    \
            std::set<std::string>{ "custom", "omnitrace", "libomnitrace",                \
                                   __VA_ARGS__ });                                       \
        if(!_ret.second)                                                                 \
        {                                                                                \
            OMNITRACE_PRINT("Warning! Duplicate setting: %s / %s\n",                     \
                            get_setting_name(ENV_NAME).c_str(), ENV_NAME);               \
        }                                                                                \
        return _config->find(ENV_NAME)->second;                                          \
    }()

namespace omnitrace
{
using settings = ::tim::settings;

inline namespace config
{
namespace
{
std::string
get_setting_name(std::string _v)
{
    static const auto _prefix = tim::string_view_t{ "omnitrace_" };
    for(auto& itr : _v)
        itr = tolower(itr);
    auto _pos = _v.find(_prefix);
    if(_pos == 0) return _v.substr(_prefix.length());
    return _v;
}
}  // namespace

void
setup_config_groups(const std::shared_ptr<::tim::settings>& _config)
{
    if(!get_env("OMNITRACE_ENABLE_CONFIG_GROUPS", false)) return;

    auto _default_config_groups_file =
        JOIN(':', JOIN('/', get_env<std::string>("HOME"), ".omnitrace.ini"),
             JOIN('/', get_env<std::string>("HOME"), ".omnitrace.toml"));

    static auto ini_configs = std::deque<toml::parse_result>{};

    OMNITRACE_PRINT_F("configuring settings\n");

    auto&& _groups_file_cb = [](tim::vsettings* _vset, std::string_view /*_val*/,
                                tim::setting_update_type /*_upd*/) {
        for(const auto& itr : tim::delimit(_vset->get<std::string>().second, " \t\n,:;"))
        {
            if(!filepath::exists(itr))
            {
                OMNITRACE_WARNING(0, "Config group file '%s' does not exist\n",
                                  itr.c_str());
                continue;
            }

            auto fitr = filepath::realpath(itr);
            try
            {
                auto config = toml::parse_file(fitr);
                ini_configs.emplace_back(std::move(config));
            } catch(std::exception& _e)
            {
                OMNITRACE_WARNING(0, "Parsing error while reading '%s': %s\n",
                                  fitr.c_str(), _e.what());
            }
        }

        for(const auto& config : ini_configs)
        {
            for(auto&& [k, v] : config)
            {
                if(v.type() == toml::node_type::string)
                {
                    const auto* _val     = v.as_string();
                    std::string _str_val = JOIN("", *_val);
                    OMNITRACE_PRINT("- %s = %s\n", k.data(), _str_val.c_str());
                }
                else if(v.type() == toml::node_type::array)
                {
                    OMNITRACE_PRINT("- %s = array\n", k.data());
                }
                else if(v.type() == toml::node_type::table)
                {
                    OMNITRACE_PRINT("- %s = table\n", k.data());
                    const auto* tbl = v.as_table();
                    if(tbl)
                    {
                        for(auto titr : *tbl)
                        {
                            if(titr.second.type() == toml::node_type::string)
                            {
                                const auto* _val     = titr.second.as_string();
                                auto        _str_val = _val->value_or(std::string{});
                                OMNITRACE_PRINT("  - %s = %s\n", titr.first.data(),
                                                _str_val.c_str());

                                set_setting_value(titr.first.data(), _str_val,
                                                  settings::update_type::config);
                            }
                            else if(titr.second.type() == toml::node_type::boolean)
                            {
                                const auto* _val     = titr.second.as_boolean();
                                std::string _str_val = JOIN("", *_val);
                                OMNITRACE_PRINT("  - %s = %s\n", titr.first.data(),
                                                _str_val.c_str());

                                set_setting_value(titr.first.data(),
                                                  *titr.second.value<bool>(),
                                                  settings::update_type::config);
                            }
                            else if(titr.second.type() == toml::node_type::array)
                            {
                                OMNITRACE_PRINT("  - %s = array\n", titr.first.data());
                            }
                            else if(titr.second.type() == toml::node_type::table)
                            {
                                OMNITRACE_PRINT("  - %s = table\n", titr.first.data());
                            }
                        }
                    }
                }
                else
                {
                    OMNITRACE_PRINT("Config: %s = %i\n", k.data(),
                                    static_cast<int>(v.type()));
                }
            }
        }
    };

    OMNITRACE_CONFIG_SETTING(
        std::string, "OMNITRACE_CONFIG_GROUPS_FILE",
        "INI / TOML file specifying groups of configuration settings", std::string{},
        _groups_file_cb, "core", "config");

    auto _config_groups_file =
        _config->at("OMNITRACE_CONFIG_GROUPS_FILE")->get<std::string>().second;
    auto _config_groups_upd = settings::update_type::env;

    // if not set via env, default to ~/.omnitrace.{toml,ini}
    if(_config_groups_file.empty())
    {
        _config_groups_file = _default_config_groups_file;
        _config_groups_upd  = settings::update_type::default_value;
    }

    // trigger the callback
    _config->at("OMNITRACE_CONFIG_GROUPS_FILE")
        ->parse(_config_groups_file, _config_groups_upd);

    auto&& _groups_cb = [](tim::vsettings* _vset, std::string_view _val,
                           tim::setting_update_type _upd) {
        tim::consume_parameters(_vset, _val, _upd);

        // auto groups = tim::delimit(_vset->get<std::string>().second, " \t\n,:;");
        // for(auto itr : groups)
        // {
        //     for(const auto& config : ini_configs)
        //     {
        //         for(auto&& [k, v] : config)
        //         {
        //             // ...
        //         }
        //     }
        // }
    };

    OMNITRACE_CONFIG_SETTING(std::string, "OMNITRACE_CONFIG_GROUPS",
                             "List of configuration groups defined in files",
                             std::string{}, _groups_cb, "core", "config");
}
}  // namespace config
}  // namespace omnitrace
