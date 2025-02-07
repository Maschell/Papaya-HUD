// SPDX-License-Identifier: GPL-3.0-or-later

#include <atomic>
#include <cstdio>

#include <sys/socket.h> // struct sockaddr
#include <wups.h>

#include "net_mon.hpp"

#include "logging.hpp"
#include "nintendo_glyphs.h"


namespace net_mon {

    std::atomic_uint bytes_received = 0;
    std::atomic_uint bytes_sent = 0;


    void
    initialize()
    {
        bytes_received = 0;
        bytes_sent = 0;
    }


    void
    finalize()
    {}


    const char*
    get_report(float dt)
    {
        static char buf[64];

        const unsigned down = std::atomic_exchange(&bytes_received, 0u);
        const unsigned up = std::atomic_exchange(&bytes_sent, 0u);

        const float down_rate = down / 1024.0f / dt;
        const float up_rate = up / 1024.0f / dt;

        std::snprintf(buf, sizeof buf,
                      NIN_GLYPH_ARROW_DOWN "%.1f KiB/s "
                      NIN_GLYPH_ARROW_UP "%.1f KiB/s",
                      down_rate,
                      up_rate);
        return buf;
    }

} // namespace net_mon


DECL_FUNCTION(int, recv,
              int fd,
              void* buf,
              int len,
              int flags)
{
    int result = real_recv(fd, buf, len, flags);
    if (result != -1)
        net_mon::bytes_received += result;
    return result;
}


DECL_FUNCTION(int, recvfrom,
              int fd,
              void* buf,
              int len,
              int flags,
              struct sockaddr* src,
              int* src_len)
{
    int result = real_recvfrom(fd, buf, len, flags, src, src_len);
    if (result != -1)
        net_mon::bytes_received += result;
    return result;
}


DECL_FUNCTION(int, recvfrom_ex,
              int fd,
              void* buf,
              int len,
              int flags,
              struct sockaddr* src,
              int* src_len,
              void* msg,
              int msg_len)
{
    int result = real_recvfrom_ex(fd, buf, len, flags, src, src_len, msg, msg_len);
    if (result != -1)
        net_mon::bytes_received += result;
    return result;
}


DECL_FUNCTION(int, recvfrom_multi,
              int fd,
              int flags,
              void* buffs,
              int data_len,
              int data_count,
              struct timeval* timeout)
{
    int result = real_recvfrom_multi(fd, flags, buffs, data_len, data_count, timeout);
    if (result != -1)
        net_mon::bytes_received += result;
    return result;
}


DECL_FUNCTION(int, send,
              int fd,
              const void* buf,
              int len,
              int flags)
{
    int result = real_send(fd, buf, len, flags);
    if (result != -1)
        net_mon::bytes_sent += result;
    return result;
}


DECL_FUNCTION(int, sendto,
              int fd,
              const void* buf,
              int len,
              int flags,
              const struct sockaddr* dst,
              int dst_len)
{
    int result = real_sendto(fd, buf, len, flags, dst, dst_len);
    if (result != -1)
        net_mon::bytes_sent += result;
    return result;
}


DECL_FUNCTION(int, sendto_multi,
              int fd,
              const void *buf,
              int len,
              int flags,
              const struct sockaddr* dstv,
              int dstv_len)
{
    int result = real_sendto_multi(fd, buf, len, flags, dstv, dstv_len);
    if (result != -1)
        net_mon::bytes_sent += result;
    return result;
}


DECL_FUNCTION(int, sendto_multi_ex,
              int fd,
              int flags,
              void* buffs,
              int count)
{
    int result = real_sendto_multi_ex(fd, flags, buffs, count);
    if (result != -1)
        net_mon::bytes_sent += result;
    return result;
}


WUPS_MUST_REPLACE(recv,           WUPS_LOADER_LIBRARY_NSYSNET, recv);
WUPS_MUST_REPLACE(recvfrom,       WUPS_LOADER_LIBRARY_NSYSNET, recvfrom);
WUPS_MUST_REPLACE(recvfrom_ex,    WUPS_LOADER_LIBRARY_NSYSNET, recvfrom_ex);
WUPS_MUST_REPLACE(recvfrom_multi, WUPS_LOADER_LIBRARY_NSYSNET, recvfrom_multi);

WUPS_MUST_REPLACE(send,            WUPS_LOADER_LIBRARY_NSYSNET, send);
WUPS_MUST_REPLACE(sendto,          WUPS_LOADER_LIBRARY_NSYSNET, sendto);
WUPS_MUST_REPLACE(sendto_multi,    WUPS_LOADER_LIBRARY_NSYSNET, sendto_multi);
WUPS_MUST_REPLACE(sendto_multi_ex, WUPS_LOADER_LIBRARY_NSYSNET, sendto_multi_ex);
