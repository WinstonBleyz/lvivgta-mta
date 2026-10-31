/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        Client/loader/Main.h
 *  PURPOSE:     MTA loader
 *
 *  Multi Theft Auto is available from https://multitheftauto.com/
 *
 *****************************************************************************/

#pragma once

#include "LvivBrand.h"

#define GTA_EXE_NAME            "gta_sa.exe"
#define PROXY_GTA_EXE_NAME      "proxy_sa.exe"
#define STEAM_GTA_EXE_NAME      "gta-sa.exe"
#define MTA_GTA_KNOWN_FILE_NAME "models\\gta3.img"
#define MTA_GUID                "Global\\{6B1E5A2C-1F0D-4C55-9A0B-7D4C1E2F3A61}"  // GTA LVIV BETA: own instance mutex (not shared with official MTA)
#define URI_CONNECT             1
#define MTA_EXE_NAME_RELEASE    LVIV_EXE_NAME

#ifdef MTA_DEBUG
    #define MTA_EXE_NAME          LVIV_EXE_NAME_DEBUG
    #define MTA_DLL_NAME          "core_d.dll"
    #define CHECK_DM_LUA_NAME     "mods\\deathmatch\\lua5.1c_d.dll"
    #define CHECK_DM_CLIENT_NAME  "mods\\deathmatch\\Client_d.dll"
    #define LOADER_PROXY_DLL_NAME "mtasa_d.dll"
#else
    #define MTA_EXE_NAME          LVIV_EXE_NAME
    #define MTA_DLL_NAME          "core.dll"
    #define CHECK_DM_LUA_NAME     "mods\\deathmatch\\lua5.1c.dll"
    #define CHECK_DM_CLIENT_NAME  "mods\\deathmatch\\Client.dll"
    #define LOADER_PROXY_DLL_NAME "mtasa.dll"
#endif
