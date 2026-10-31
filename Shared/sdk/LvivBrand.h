/*****************************************************************************
 *
 *  PROJECT:     GTA LVIV BETA (custom build of Multi Theft Auto: San Andreas)
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        Shared/sdk/LvivBrand.h
 *  PURPOSE:     Single place for the custom client's branding and the
 *               official-server allowlist. Only affects this custom build.
 *
 *****************************************************************************/

#pragma once

#include <cstring>

// Branding
#define LVIV_PRODUCT_NAME       "GTA LVIV BETA"
#define LVIV_PRODUCT_NAME_W     L"GTA LVIV BETA"
#define LVIV_EXE_NAME           "GTA LVIV BETA.exe"
#define LVIV_EXE_NAME_W         L"GTA LVIV BETA.exe"
#define LVIV_EXE_NAME_DEBUG     "GTA LVIV BETA_d.exe"
#define LVIV_EXE_NAME_DEBUG_W   L"GTA LVIV BETA_d.exe"

// The only server this client may connect to
#define LVIV_SERVER_HOST        "149.50.108.108"
#define LVIV_SERVER_PORT        28570

// UTF-8 bytes (sources are compiled as MBCS): "GTA LVIV BETA підтримує підключення лише до офіційного сервера проєкту"
#define LVIV_BLOCKED_MESSAGE \
    "GTA LVIV BETA \xD0\xBF\xD1\x96\xD0\xB4\xD1\x82\xD1\x80\xD0\xB8\xD0\xBC\xD1\x83\xD1\x94 \xD0\xBF\xD1\x96\xD0\xB4\xD0\xBA\xD0\xBB\xD1\x8E\xD1\x87\xD0\xB5\xD0\xBD\xD0\xBD\xD1\x8F \xD0\xBB\xD0\xB8\xD1\x88\xD0\xB5 \xD0\xB4\xD0\xBE \xD0\xBE\xD1\x84\xD1\x96\xD1\x86\xD1\x96\xD0\xB9\xD0\xBD\xD0\xBE\xD0\xB3\xD0\xBE \xD1\x81\xD0\xB5\xD1\x80\xD0\xB2\xD0\xB5\xD1\x80\xD0\xB0 \xD0\xBF\xD1\x80\xD0\xBE\xD1\x94\xD0\xBA\xD1\x82\xD1\x83"
// UTF-8 bytes: "Підключення до LVIV GTA BETA"
#define LVIV_CONNECTING_MESSAGE \
    "\xD0\x9F\xD1\x96\xD0\xB4\xD0\xBA\xD0\xBB\xD1\x8E\xD1\x87\xD0\xB5\xD0\xBD\xD0\xBD\xD1\x8F \xD0\xB4\xD0\xBE LVIV GTA BETA"

// Address must be the numeric IPv4 of the official server (callers resolve host names first)
inline bool LvivIsAllowedEndpoint(const char* szIPv4, unsigned short usPort)
{
    return szIPv4 && usPort == LVIV_SERVER_PORT && std::strcmp(szIPv4, LVIV_SERVER_HOST) == 0;
}
