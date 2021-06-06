// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xconfig/xconfig_face.h"

NS_BEG2(top, config)

/**
 *
 *  config helper utility
 *
 */
class xtop_config_utility {
public:
    /**
     * @brief incremental add onchain black/whitelist param
     *
     * @param bwlist  the whitelist or blacklist
     * @param value the proposal target value
     * @return the new white/black list
     *
     */
    static std::string incremental_add_bwlist(std::string const& bwlist, std::string const& value);

    /**
     * @brief incremental delete onchain black/whitelist param
     *
     * @param bwlist the whitelist or blacklist
     * @param value the proposal target value
     * @return the new white/black list
     *
     */
    static std::string incremental_delete_bwlist(std::string const& bwlist, std::string const& value);
};

using xconfig_utl = xtop_config_utility;


class xconfig_update_parameter_action_t final : public  xconfig_update_action_face_t
{
public:
    bool do_update(const std::map<std::string, std::string>& params) override;
};

class xconfig_incremental_add_update_parameter_action_t final : public xconfig_update_action_face_t {
public:
    bool do_update(const std::map<std::string, std::string> & params) override;
};


class xconfig_incremental_delete_update_parameter_action_t final : public xconfig_update_action_face_t {
public:
    bool do_update(const std::map<std::string, std::string>& params) override;
};

class xconfig_add_parameter_action_t final : public xconfig_update_action_face_t {
public:
    bool do_update(const std::map<std::string, std::string>& params) override;
};

class xconfig_delete_parameter_action_t final : public xconfig_update_action_face_t {
public:
    bool do_update(const std::map<std::string, std::string>& params) override;
};

NS_END2
