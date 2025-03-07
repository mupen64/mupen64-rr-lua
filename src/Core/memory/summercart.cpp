/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include <Core.h>
#include <memory/memory.h>
#include <memory/summercart.h>
#include <r4300/rom.h>

struct vhd {
    char cookie[8];
    uint32_t feature;
    uint32_t version;
    uint64_t next;
    uint32_t stamp;
    char creator_app[4];
    uint32_t creator_version;
    char creator_os[4];
    uint64_t disk_size;
    uint64_t data_size;
    uint16_t ncylinder;
    unsigned char nhead;
    unsigned char nsector;
    uint32_t type;
    uint32_t checksum;
    char guid[16];
    unsigned char saved_state;
    char pad[427];
};

#ifdef _BIG_ENDIAN
#define vhd_16(val) (val)
#define vhd_32(val) (val)
#define vhd_64(val) (val)
#else
#define vhd_16(val) _byteswap_ushort(val)
#define vhd_32(val) _byteswap_ulong(val)
#define vhd_64(val) _byteswap_uint64(val)
#endif

static void vhd_copy(struct vhd* vhd, FILE* dst, FILE* src, void* buf, uint32_t n)
{
    uint64_t len;
    fseek(src, -512, SEEK_END);
    fread(vhd, 1, sizeof(struct vhd), src);
    fseek(src, 0, SEEK_SET);
    for (len = vhd_64(vhd->disk_size) / 512; len > 0; len -= n)
    {
        if (n > len)
            n = len;
        fread(buf, n, 512, src);
        fwrite(buf, n, 512, dst);
    }
}

struct summercart summercart;

static int32_t sd_error(const wchar_t* text, const wchar_t* caption)
{
    g_core->show_dialog(text, caption, fsvc_error);
    return -1;
}

static int32_t sd_seek(FILE* fp, const wchar_t* caption)
{
    struct vhd vhd;
    uint32_t sector = summercart.sd_sector;
    uint32_t count = summercart.data1;
    if (fseek(fp, -512, SEEK_END))
        return sd_error(L"Seek(1) error.", caption);
    if (fread(&vhd, 1, sizeof(struct vhd), fp) != sizeof(struct vhd))
        return sd_error(L"Read error.", caption);
    if (memcmp(vhd.cookie, "conectix", 8))
        return sd_error(L"Invalid VHD file.", caption);
    if (vhd_32(vhd.type) != 2)
        return sd_error(L"Invalid VHD type: must be a fixed disk.", caption);
    if ((int64_t)sector + count > vhd_64(vhd.disk_size) / 512)
        return -1;
    if (fseek(fp, 512 * (int64_t)sector, SEEK_SET))
        return sd_error(L"Seek(2) error.", caption);
    return 0;
}

static void sd_read()
{
    uint32_t i;
    FILE* fp;
    const auto path = g_core->get_summercart_path();
    char* ptr = NULL;
    uint32_t addr = summercart.data0 & 0x1fffffff;
    uint32_t count = summercart.data1;
    uint32_t size = 512 * count;

    if (count > 131072)
        return;

    if ((fp = fopen(path.string().c_str(), "rb")))
    {
        char s = S8;
        if (!sd_seek(fp, L"SD read error"))
        {
            if (addr >= 0x1ffe0000 && addr + size <= 0x1ffe2000)
            {
                addr -= 0x1ffe0000;
                ptr = summercart.buffer;
            }
            if (addr >= 0x10000000 && addr + size <= 0x14000000)
            {
                s ^= summercart.sd_byteswap;
                addr -= 0x10000000;
                ptr = (char*)rom;
            }
        }
        if (ptr)
        {
            for (i = 0; i < size; i++)
                ptr[(addr + i) ^ s] = fgetc(fp);
            summercart.status = 0;
        }
        fclose(fp);
    }
    else
        sd_error(L"Could not open SD image file.", L"SD read error");
}

static void sd_write()
{
    uint32_t i;
    FILE* fp;
    const auto path = g_core->get_summercart_path();
    char* ptr = NULL;
    uint32_t addr = summercart.data0 & 0x1fffffff;
    uint32_t count = summercart.data1;
    uint32_t size = 512 * count;

    if (count > 131072)
        return;

    if ((fp = fopen(path.string().c_str(), "r+b")))
    {
        if (!sd_seek(fp, L"SD write error"))
        {
            if (addr >= 0x1ffe0000 && addr + size <= 0x1ffe2000)
            {
                addr -= 0x1ffe0000;
                ptr = summercart.buffer;
            }
            if (addr >= 0x10000000 && addr + size <= 0x14000000)
            {
                addr -= 0x10000000;
                ptr = (char*)rom;
            }
        }
        if (ptr)
        {
            for (i = 0; i < size; i++)
                fputc(ptr[(addr + i) ^ S8], fp);
            summercart.status = 0;
        }
        fclose(fp);
    }
    else
        sd_error(L"Could not open SD image file.", L"SD write error");
}

void save_summercart(const std::filesystem::path& path)
{
    uint32_t n;
    void* buf;
    FILE* sdf;
    FILE* stf;
    struct vhd vhd;
    const auto smc_path = g_core->get_summercart_path();

    if ((buf = malloc(512 * (n = 128))))
    {
        if ((sdf = fopen(smc_path.string().c_str(), "rb")))
        {
            if ((stf = fopen(path.string().c_str(), "wb")))
            {
                vhd_copy(&vhd, stf, sdf, buf, n);
                fwrite(&summercart, 1, sizeof(struct summercart), stf);
                fwrite(&vhd, 1, sizeof(struct vhd), stf);
                fclose(stf);
            }
            else
                sd_error(L"Could not open SD state file.", L"Save error");
            fclose(sdf);
        }
        else
            sd_error(L"Could not open SD image file.", L"Save error");
    }
    else
        sd_error(L"Could not allocate buffer.", L"Save error");
}

void load_summercart(const std::filesystem::path& path)
{
    uint32_t n;
    void* buf;
    FILE* stf;
    FILE* sdf;
    struct vhd vhd;
    const auto smc_path = g_core->get_summercart_path();
    if ((buf = malloc(512 * (n = 128))))
    {
        if ((stf = fopen(smc_path.string().c_str(), "rb")))
        {
            if ((sdf = fopen(path.string().c_str(), "wb")))
            {
                vhd_copy(&vhd, sdf, stf, buf, n);
                fread(&summercart, 1, sizeof(struct summercart), stf);
                fwrite(&vhd, 1, sizeof(struct vhd), sdf);
                fclose(sdf);
            }
            else
                sd_error(L"Could not open SD image file.", L"Load error");
            fclose(stf);
        }
        else
            sd_error(L"Could not open SD state file.", L"Load error");
        free(buf);
    }
    else
        sd_error(L"Could not allocate buffer.", L"Load error");
}

void init_summercart()
{
    memset(&summercart, 0, sizeof(struct summercart));
}

uint32_t read_summercart(uint32_t address)
{
    switch (address & 0xFFFC)
    {
    case 0x00:
        if (summercart.unlock)
            return summercart.status;
        break;
    case 0x04:
        if (summercart.unlock)
            return summercart.data0;
        break;
    case 0x08:
        if (summercart.unlock)
            return summercart.data1;
        break;
    case 0x0C:
        if (summercart.unlock)
            return 0x53437632;
        break;
    }
    return 0;
}

void write_summercart(uint32_t address, uint32_t value)
{
    switch (address & 0xFFFC)
    {
    case 0x00:
        if (summercart.unlock)
        {
            summercart.status = 0x40000000;
            switch (value)
            {
            case 'c':
                switch (summercart.data0)
                {
                case 1:
                    summercart.data1 = summercart.cfg_rom_write;
                    summercart.status = 0;
                    break;
                case 3:
                    summercart.data1 = 0;
                    summercart.status = 0;
                    break;
                case 6:
                    summercart.data1 = 0;
                    summercart.status = 0;
                    break;
                }
                break;
            case 'C':
                switch (summercart.data0)
                {
                case 1:
                    if (summercart.data1)
                    {
                        summercart.data1 = summercart.cfg_rom_write;
                        summercart.cfg_rom_write = 1;
                    }
                    else
                    {
                        summercart.data1 = summercart.cfg_rom_write;
                        summercart.cfg_rom_write = 0;
                    }
                    summercart.status = 0;
                    break;
                }
                break;
            case 'i':
                switch (summercart.data1)
                {
                case 0:
                    summercart.status = 0;
                    break;
                case 1:
                    summercart.status = 0;
                    break;
                case 4:
                    summercart.sd_byteswap = 1;
                    summercart.status = 0;
                    break;
                case 5:
                    summercart.sd_byteswap = 0;
                    summercart.status = 0;
                    break;
                }
                break;
            case 'I':
                summercart.sd_sector = summercart.data0;
                summercart.status = 0;
                break;
            case 's':
                sd_read();
                break;
            case 'S':
                sd_write();
                break;
            }
        }
        break;
    case 0x04:
        if (summercart.unlock)
            summercart.data0 = value;
        break;
    case 0x08:
        if (summercart.unlock)
            summercart.data1 = value;
        break;
    case 0x10:
        switch (value)
        {
        case 0xFFFFFFFF:
            summercart.unlock = 0;
            break;
        case 0x5F554E4C:
            if (summercart.lock_seq == 0)
            {
                summercart.lock_seq = 2;
            }
            break;
        case 0x4F434B5F:
            if (summercart.lock_seq == 2)
            {
                summercart.unlock = 1;
                summercart.lock_seq = 0;
            }
            break;
        default:
            summercart.lock_seq = 0;
            break;
        }
        break;
    }
}
