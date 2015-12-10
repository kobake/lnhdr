#pragma once

#include "stdafx.h"

enum LinkType{
	LINK_JUNCTION = 0,
	LINK_HARDLINK = 1,
	LINK_SYMLINK = 2,
	LINK_AUTO = 99,
};

BOOL CreateLink(const wchar_t *link_, const wchar_t *target_, LinkType linkType);
BOOL Unlink(const wchar_t *filename);
BOOL GetLinkTargetPath(wchar_t *buffer, const wchar_t *filename);
BOOL IsSymlink(const wchar_t *filename);

void GetNormalizedPath(wchar_t * buffer, const wchar_t *src);
void CheckTargetAttributes(const wchar_t* target_, bool* enableJunction, bool* enableHardlink, bool* enableSymlink);
void CreateUniqueName(wchar_t *buffer, const wchar_t *dir, const wchar_t *src, int nResId);

bool SymlinkCopyMove(LPCTSTR pszSrcFile, LPCTSTR pszDestFile, bool fMove);

typedef enum {FI_CREATED = 0, FI_DELETED = 1} USFI_STATUS;
void UpdateShell(const wchar_t *filename, bool isFolder, USFI_STATUS status);

typedef enum {FIT_FOLDER = 0, FIT_MOUNTPOINT = 1, FIT_SYMLINK = 2} FOLDER_TYPE;
FOLDER_TYPE GetSymlinkType(const wchar_t *filename);
