// SPDX-License-Identifier: GPL-3.0-or-later

#include <atomic>
#include <cstdio>

#include <coreinit/filesystem.h>
#include <coreinit/filesystem_fsa.h>

#include <wups.h>
#include <malloc.h>

#include "fs_mon.hpp"

namespace fs_mon {
    std::atomic_uint bytes_read = 0;
    std::atomic_uint bytes_written = 0;
    std::atomic_uint read_count = 0;
    std::atomic_uint write_count = 0;

    void
    initialize() {
       bytes_read = 0;
       bytes_written = 0;
       read_count = 0;
       write_count = 0;
    }

    void
    finalize() {}

    const char *
    get_report(float dt) {
       static char buf[64];

       const unsigned read = std::atomic_exchange(&bytes_read, 0u);
       const float read_rate = read / (1024.0f * 1024.0f) / dt;
       const unsigned write = std::atomic_exchange(&bytes_written, 0u);
       const float write_rate = write / (1024.0f * 1024.0f) / dt;
       const unsigned read_cnt = std::atomic_exchange(&read_count, 0u);
       const float read_count_rate = read_cnt / dt;
       const unsigned write_cnt = std::atomic_exchange(&write_count, 0u);
       const float write_count_rate = write_cnt / dt;

       std::snprintf(buf, sizeof buf,
                     "read %.1f MiB/s | write %.1f MiB/s | reads %0.1f/s | writes %0.1f/s",
                     read_rate,
                     write_rate,
                     read_count_rate,
                     write_count_rate);
       return buf;
    }

} // namespace fs_mon

void CalcReadWriteInfo(FSAShimBuffer *shim, FSError res) {
   if (res < 0) {
      return;
   }

   if (shim->command == FSA_COMMAND_RAW_READ) {
      fs_mon::read_count++;
      fs_mon::bytes_read += shim->request.rawRead.size * res;
   } else if (shim->command == FSA_COMMAND_RAW_WRITE) {
      fs_mon::write_count++;
      fs_mon::bytes_written += shim->request.rawWrite.size * res;
   } else if (shim->command == FSA_COMMAND_READ_FILE) {
      fs_mon::read_count++;
      fs_mon::bytes_read += shim->request.readFile.size * res;
   } else if (shim->command == FSA_COMMAND_WRITE_FILE) {
      fs_mon::write_count++;
      fs_mon::bytes_written += shim->request.writeFile.size * res;
   }
}

DECL_FUNCTION(FSError, fsaShimSubmitRequest,
              FSAShimBuffer *shim,
              FSError emulatedError) {
   auto res = real_fsaShimSubmitRequest(shim, emulatedError);
   CalcReadWriteInfo(shim, res);
   return res;
}

struct ReadFileContextWrapper {
    IOSAsyncCallbackFn realCallback;
    void *realContext;
    FSAShimBuffer *shim;
};

#define FSAShimDecodeIosErrorToFsaStatus     ((FSError (*)(IOSHandle  handle, IOSError error))(0x101C400 + 0x42bc4))

void asyncCallbackTest(IOSError result, void *context) {
   auto *wrapper = static_cast<ReadFileContextWrapper *>(context);
   CalcReadWriteInfo(wrapper->shim, FSAShimDecodeIosErrorToFsaStatus(wrapper->shim->clientHandle, result));
   if (wrapper->realCallback) {
      wrapper->realCallback(result, wrapper->realContext);
   }
   free(wrapper);
}

DECL_FUNCTION(FSError, fsaShimSubmitRequestAsync, FSAShimBuffer *shim,
              FSError emulatedError,
              IOSAsyncCallbackFn callback,
              void *context) {
   if (shim->command != FSA_COMMAND_READ_FILE) {
      return real_fsaShimSubmitRequestAsync(shim, emulatedError, callback, context);
   }
   auto *wrapper = static_cast<ReadFileContextWrapper *>(memalign(0x40, sizeof(ReadFileContextWrapper)));
   if (!wrapper) {
      return real_fsaShimSubmitRequestAsync(shim, emulatedError, callback, context);
   }
   *wrapper = {};
   wrapper->realCallback = callback;
   wrapper->realContext = context;
   wrapper->shim = shim;

   auto result = real_fsaShimSubmitRequestAsync(shim, emulatedError, asyncCallbackTest, wrapper);
   if (result != FS_ERROR_OK) {
      free(wrapper);
      return real_fsaShimSubmitRequestAsync(shim, emulatedError, callback, context);
   }

   return result;
}

WUPS_MUST_REPLACE_PHYSICAL(fsaShimSubmitRequest, (0x02042d90 + 0x3001c400), (0x02042d90 - 0xfe3c00));
WUPS_MUST_REPLACE_PHYSICAL(fsaShimSubmitRequestAsync, (0x02042e84 + 0x3001c400), (0x02042e84 - 0xfe3c00));
