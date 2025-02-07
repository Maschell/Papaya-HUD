// SPDX-License-Identifier: GPL-3.0-or-later

#include "storage_error.hpp"


namespace wups::storage {

    storage_error::storage_error(const std::string& msg,
                                 WUPSStorageError status) :
        std::runtime_error{msg + ": " +
                           std::string(WUPSStorageAPI::GetStatusStr(status))},
        code{status}
    {}

}
