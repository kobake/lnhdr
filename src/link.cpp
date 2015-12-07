//---------------------------------------------------------------------------
// Reparse Point Operation Module
// Copyright and Programmed by T.Kawasaki
//---------------------------------------------------------------------------
// このコードはあるがままに提供され、無保証です。
// このプログラムにある著作権表示を省略しない限り、このプログラムの一部、ある
// いはすべてを改変の有無に限らず利用することができます。
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "link.h"
#include "rsprintf.h"
#include "resource.h"

//
// Undocumented FSCTL_SET_REPARSE_POINT structure definition
//

#define MAX_REPARSE_SIZE 17000
#define REPARSE_MOUNTPOINT_HEADER_SIZE   8

typedef struct {
    DWORD          ReparseTag;
    WORD           ReparseDataLength;
    WORD           Reserved;

    WORD           TargetNameOffset;
    WORD           TargetNameLength;
    WORD           PrintNameOffset;
    WORD           PrintNameLength;
    WCHAR          PathBuffer[1];           // ?
} REPARSE_MOUNTPOINT_DATA_BUFFER, *PREPARSE_MOUNTPOINT_DATA_BUFFER;

#undef FSCTL_SET_REPARSE_POINT
#undef FSCTL_GET_REPARSE_POINT
#undef FSCTL_DELETE_REPARSE_POINT
#define FSCTL_SET_REPARSE_POINT \
	CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 41, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_GET_REPARSE_POINT \
	CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 42, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DELETE_REPARSE_POINT \
	CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 43, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IORPS_TAG_SYMBOLIC_LINK             (0x80000000)
#define IORPS_TAG_MOUNT_POINT               (0xA0000003)
#define IORPS_TAG_HSM                       (0xC0000004)
#define IORPS_TAG_NSS                       (0x80000005)
#define IORPS_TAG_NSSRECOVER                (0x80000006)
#define IORPS_TAG_SIS                       (0x80000007)
#define IORPS_TAG_DFS                       (0x80000008)

void GetNormalizedPath(wchar_t * buffer, const wchar_t *src)
{
	PathCanonicalizeW(buffer, src);
	*PathFindFileNameW(buffer) = 0;
	PathRemoveBackslashW(buffer);
}

BOOL Unlink(const wchar_t *filename)
{
	BOOL f;
	if(GetFileAttributesW(filename) & FILE_ATTRIBUTE_DIRECTORY)
	{
		f = RemoveDirectoryW(filename);
		if(f) UpdateShell(filename, true, FI_DELETED);
	}
	else
	{
		f = DeleteFileW(filename);
		if(f) UpdateShell(filename, false, FI_DELETED);
	}
	return f;
}

static BOOL CreateJunction(const wchar_t *linkname, const wchar_t *targetFileName)
{
	char   reparseBuffer[MAX_PATH*3];
	WCHAR  targetNativeFileName[MAX_PATH];
	HANDLE hFile;
	DWORD  returnedLength;
	PREPARSE_MOUNTPOINT_DATA_BUFFER reparseInfo
		= (PREPARSE_MOUNTPOINT_DATA_BUFFER) reparseBuffer;
	BOOL fDirectory
		= GetFileAttributes(targetFileName) & FILE_ATTRIBUTE_DIRECTORY;

	//
	// Make the native target name
	//
	if(fDirectory)
	{
		wchar_t fn[MAX_PATH];
		lstrcpy(fn, targetFileName);
		if(fn[lstrlenW(fn) - 1] != '\\')
			lstrcat(fn, L"\\");

		if(GetVolumeNameForVolumeMountPointW(
			fn, targetNativeFileName, MAX_PATH))
		{
			// Create DRIVE-MOUNTPOINT
			// It's needed to do it directly..., I don't know why.
			// Although SetVolumeMountPoint requires "\\?\Volume{GUID}\" form,
			// ReparsePoint does "\??\Volume{GUID}\".
			targetNativeFileName[1] = '?';
		}
		else
		{
			wsprintfW(targetNativeFileName, L"\\??\\%s", fn);
			PathRemoveBackslash(targetNativeFileName);
		}
	}
	else
	{
		wsprintfW(targetNativeFileName, L"\\??\\%s", targetFileName);
	}

	//
	// Create the link - ignore errors since it might already exist
	//
	if(fDirectory)
	{
		CreateDirectoryW(linkname, NULL);
		hFile = CreateFileW(
			linkname,
			GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS,
			NULL);
	}
	else
	{
		// Never used
		hFile = CreateFileW(
			linkname,
			GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			CREATE_NEW,
			FILE_FLAG_OPEN_REPARSE_POINT,
			NULL);
	}
	if(hFile == INVALID_HANDLE_VALUE)
		return FALSE;

	//
	// Build the reparse info
	//
	memset(reparseInfo, 0, sizeof(*reparseInfo));
	if(fDirectory)
		reparseInfo->ReparseTag = IORPS_TAG_MOUNT_POINT;
	else
		reparseInfo->ReparseTag = IORPS_TAG_SYMBOLIC_LINK; // Never used

	reparseInfo->TargetNameOffset = 0;
	reparseInfo->TargetNameLength
		= (WORD)(lstrlenW(targetNativeFileName) * sizeof(wchar_t));

	reparseInfo->PrintNameOffset
		= reparseInfo->TargetNameLength + sizeof(wchar_t);
	reparseInfo->PrintNameLength = 0;
	memset((char *)reparseInfo + reparseInfo->PrintNameOffset, 0, 4);

	lstrcpyW(reparseInfo->PathBuffer, targetNativeFileName);

	// ReparseDataLength is the length of data storage
	reparseInfo->ReparseDataLength
		= REPARSE_MOUNTPOINT_HEADER_SIZE
		+ reparseInfo->PrintNameOffset
		+ reparseInfo->PrintNameLength + 2;

	//
	// Set the link
	//
	if(!DeviceIoControl(hFile, FSCTL_SET_REPARSE_POINT,
				reparseInfo,
				reparseInfo->ReparseDataLength + REPARSE_MOUNTPOINT_HEADER_SIZE,
				NULL, 0, &returnedLength, NULL))
	{
		CloseHandle(hFile);
		Unlink(linkname);
		return FALSE;
	}
	CloseHandle(hFile);
	return TRUE;
}

void CreateUniqueName(wchar_t *buffer, const wchar_t *dir, const wchar_t *src, int nResId)
{
	wchar_t fn[MAX_PATH], ext[20];
	wchar_t *p;
	int n;

	if(src[1] == ':' && src[2] == '\\' && src[3] == 0)
	{
		// Drive Root
		DWORD dwCompLen, dwFSFlags;
		wchar_t volname[MAX_PATH] = L"";
		GetVolumeInformation(
			src,
			volname,
			MAX_PATH,
			NULL,
			&dwCompLen,
			&dwFSFlags,
			NULL,
			0);

		if(volname[0]) // "なんちゃら (%c)"
			lstrcpyW(fn, rsprintf(IDS_STRING122, volname, src[0]).c_str());
		else // "%c ドライブ"
			lstrcpyW(fn, rsprintf(IDS_STRING120, src[0]).c_str());
		ext[0] = 0;
	}
	else
	{
		// Directory / file
		lstrcpyW(fn, PathFindFileNameW(src));
		if(GetFileAttributes(src) & FILE_ATTRIBUTE_DIRECTORY)
			ext[0] = 0;
		else
		{
			p = PathFindExtensionW(fn);
			lstrcpy(ext, p);
			*p = 0;
		}
	}

	// "DIR/FN へのリンク.EXT"
	wsprintfW(buffer, L"%s\\%s%s", dir, fn, ext);

	for(n = 1; PathFileExistsW(buffer); n++)
	{
		// "DIR/FN へのリンク (N).EXT"
		lstrcpyW(buffer, rsprintf(nResId, n, dir, fn, ext).c_str());
	}
}

BOOL CreateLink(const wchar_t *link_, const wchar_t *target_)
{
	WCHAR target[MAX_PATH];
	WCHAR link[MAX_PATH];
	wchar_t* filePart;
	DWORD dwAttrs;

	// Get the full path referenced by the target
	if(!GetFullPathNameW(target_, MAX_PATH, target, &filePart))
		return FALSE;

	// Get the full path referenced by the directory
	if(!GetFullPathNameW(link_, MAX_PATH, link, &filePart))
		return FALSE;

	// Check File exists.
	dwAttrs = GetFileAttributesW(target);
	if(dwAttrs == (DWORD)-1)
		return FALSE;

	BOOL f;
	if(dwAttrs & FILE_ATTRIBUTE_DIRECTORY)
	{
		f = CreateJunction(link, target);
		if(f) UpdateShell(link, true, FI_CREATED);
		return f;
	}
	else
	{
		f = CreateHardLinkW(link, target, NULL);
		if(f) UpdateShell(link, false, FI_CREATED);
		return f;
	}
}

BOOL GetLinkTargetPath(wchar_t *buffer, const wchar_t *filename)
{
	// リパースバッファの確保
	DWORD dwSize;
	BYTE  reparseBuffer[MAX_REPARSE_SIZE];
	PREPARSE_MOUNTPOINT_DATA_BUFFER data
		= (PREPARSE_MOUNTPOINT_DATA_BUFFER)reparseBuffer;
	HANDLE hFile;

	// デフォルトでは、何も返さない
	buffer[0] = 0;

	// リパースポイントがなかったら何もしない
	if(!(GetFileAttributesW(filename) & FILE_ATTRIBUTE_REPARSE_POINT))
		return FALSE;

	// リパースポイントを開く
	hFile = CreateFileW(
		filename,
		0,
		FILE_SHARE_READ|FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
		NULL);
	if(hFile == INVALID_HANDLE_VALUE)
		return FALSE;

	// 情報取得
	if(!DeviceIoControl(
		hFile,
		FSCTL_GET_REPARSE_POINT,
		NULL,
		0,
		data,
		sizeof(reparseBuffer),
		&dwSize, NULL)
	)
	{
		CloseHandle(hFile);
		return FALSE;
	}
	CloseHandle(hFile);

	if(IsReparseTagMicrosoft(data->ReparseTag))
	{
		switch(data->ReparseTag)
		{
		case IORPS_TAG_SYMBOLIC_LINK:
		case IORPS_TAG_MOUNT_POINT:
			{
				// シンボリックリンク
				wchar_t *p
					= (wchar_t *)((BYTE *)&data->PathBuffer
					 + data->TargetNameOffset);
				int len = data->TargetNameLength;

				// "\??\" で始まっていたら削除
				if(wcsncmp(p, L"\\??\\", 4) == 0)
					p += 4, len -= 4;
				p[len] = 0;

				lstrcpyW(buffer, p);
				PathRemoveBackslashW(buffer);
			}
			return TRUE;
		}
	}
	return FALSE;
}

BOOL IsSymlink(const wchar_t *filename)
{
	if(!GetSymlinkType(filename))
		return FALSE;
	return TRUE;
}

FOLDER_TYPE GetSymlinkType(const wchar_t *filename)
{
	DWORD dwSize;
	BYTE  reparseBuffer[MAX_REPARSE_SIZE];
	PREPARSE_MOUNTPOINT_DATA_BUFFER data
		= (PREPARSE_MOUNTPOINT_DATA_BUFFER)reparseBuffer;
	HANDLE hFile;

	if(!(GetFileAttributesW(filename) & FILE_ATTRIBUTE_REPARSE_POINT))
		return FIT_FOLDER;

	hFile = CreateFileW(
		filename,
		0,
		FILE_SHARE_READ|FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
		NULL);
	if(hFile == INVALID_HANDLE_VALUE)
		return FIT_FOLDER;

	if(!DeviceIoControl(
		hFile,
		FSCTL_GET_REPARSE_POINT,
		NULL,
		0,
		data,
		sizeof(reparseBuffer),
		&dwSize, NULL)
	)
	{
		CloseHandle(hFile);
		return FIT_FOLDER;
	}

	CloseHandle(hFile);

	if(data->ReparseTag == IORPS_TAG_SYMBOLIC_LINK)
		return FIT_SYMLINK;
	if(data->ReparseTag == IORPS_TAG_MOUNT_POINT)
		return FIT_MOUNTPOINT;
	return FIT_FOLDER;
}

// コピー/移動
bool SymlinkCopyMove(LPCTSTR pszSrcFile, LPCTSTR pszDestFile, bool fMove)
{
	wchar_t buffer[MAX_PATH];
	if(!GetLinkTargetPath(buffer, pszSrcFile))
		return false;

	bool f = (CreateLink(pszDestFile, buffer) != 0);

	if(fMove)
		Unlink(pszSrcFile);

	return f;
}

void UpdateShell(const wchar_t *filename, bool isFolder, USFI_STATUS status)
{
	const LONG eventId[]
		= {SHCNE_CREATE, SHCNE_MKDIR, SHCNE_DELETE, SHCNE_RMDIR};
	SHChangeNotify(
		eventId[status * 2 + isFolder],
		SHCNF_PATH | SHCNF_FLUSHNOWAIT,
		filename,
		NULL);
}
