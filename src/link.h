#ifndef _LINK_H_
#define _LINK_H_

#include "stdafx.h"

#ifdef __cplusplus
extern "C" {
#endif

BOOL CreateLink(const wchar_t *link_, const wchar_t *target_);
BOOL Unlink(const wchar_t *filename);
BOOL GetLinkTargetPath(wchar_t *buffer, const wchar_t *filename);
BOOL IsSymlink(const wchar_t *filename);

void GetNormalizedPath(wchar_t * buffer, const wchar_t *src);
void CreateUniqueName(wchar_t *buffer, const wchar_t *dir, const wchar_t *src, int nResId);

bool SymlinkCopyMove(LPCTSTR pszSrcFile, LPCTSTR pszDestFile, bool fMove);

typedef enum {FI_CREATED = 0, FI_DELETED = 1} USFI_STATUS;
void UpdateShell(const wchar_t *filename, bool isFolder, USFI_STATUS status);

typedef enum {FIT_FOLDER = 0, FIT_MOUNTPOINT = 1, FIT_SYMLINK = 2} FOLDER_TYPE;
FOLDER_TYPE GetSymlinkType(const wchar_t *filename);

#ifdef __cplusplus
}
#endif

#endif // _LINK_H_
