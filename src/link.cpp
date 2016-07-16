//---------------------------------------------------------------------------
// Reparse Point Operation Module
// Copyright and Programmed by T.Kawasaki
//---------------------------------------------------------------------------
// ���̃R�[�h�͂��邪�܂܂ɒ񋟂���A���ۏ؂ł��B
// ���̃v���O�����ɂ��钘�쌠�\�����ȗ����Ȃ�����A���̃v���O�����̈ꕔ�A����
// ���͂��ׂĂ����ς̗L���Ɍ��炸���p���邱�Ƃ��ł��܂��B
//---------------------------------------------------------------------------

#include "stdafx.h"
#include "link.h"
#include "rsprintf.h"
#include "resource.h"
#include <vector>
#include <string>

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

// �^�[�Q�b�g�����̃`�F�b�N.
void CheckTargetAttributes(const wchar_t* target_, bool* enableJunction, bool* enableHardlink, bool* enableSymlink)
{
	WCHAR target[MAX_PATH];
	wchar_t* filePart;

	// Get target attributes.
	if(!GetFullPathNameW(target_, MAX_PATH, target, &filePart))return;
	DWORD dwAttrs = GetFileAttributesW(target);
	if(dwAttrs == (DWORD)-1)return;

	// Resolve type.
	*enableJunction = *enableHardlink = *enableSymlink = true;
	if(dwAttrs & FILE_ATTRIBUTE_DIRECTORY){
		*enableHardlink = false; // �f�B���N�g���̏ꍇ�̓n�[�h�����N�s��.
	}
	else{
		*enableJunction = false; // �t�@�C���̏ꍇ�̓W�����N�V�����s��.
	}
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

		if(volname[0]) // "�Ȃ񂿂�� (%c)"
			lstrcpyW(fn, rsprintf(IDS_STRING122, volname, src[0]).c_str());
		else // "%c �h���C�u"
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

	// "DIR/FN �ւ̃����N.EXT"
	wsprintfW(buffer, L"%s\\%s%s", dir, fn, ext);

	for(n = 1; PathFileExistsW(buffer); n++)
	{
		// "DIR/FN �ւ̃����N (N).EXT"
		lstrcpyW(buffer, rsprintf(nResId, n, dir, fn, ext).c_str());
	}
}

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

std::wstring linkexepath()
{
	// DLL�p�X.
	wchar_t path[_MAX_PATH];
	::GetModuleFileNameW((HINSTANCE)&__ImageBase, path, _countof(path));

	// DLL�f�B���N�g���p�X.
	wchar_t* p = wcsrchr(path, '\\');
	if(p)*p = L'\0';

	// lnhdrlink.exe�p�X.
	wcscat(path, L"\\lnhdrlink.exe");
	return path;
}

BOOL CreateLink(const wchar_t *link_, const wchar_t *target_, LinkType linkType)
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

	// Resolve type.
	bool isDir = ((dwAttrs & FILE_ATTRIBUTE_DIRECTORY) != 0);
	if(linkType == LINK_AUTO)
	{
		if(GetSymlinkType(target) == FIT_SYMLINK)
		{
			linkType = LINK_SYMLINK;
		}
		else if(isDir)
		{
			linkType = LINK_JUNCTION;
		}
		else
		{
			linkType = LINK_HARDLINK;
		}
	}

	// Link.
	if(linkType == LINK_JUNCTION)
	{
		// ���W�����N�V�����̓f�B���N�g���̂ݗL��.
		if(!isDir)return FALSE;
		BOOL f = CreateJunction(link, target);
		if(f) UpdateShell(link, true, FI_CREATED);
		return f;
	}
	else if(linkType == LINK_HARDLINK)
	{
		// ���n�[�h�����N�̓t�@�C���̂ݗL��.
		if(isDir)return FALSE;
		BOOL f = CreateHardLinkW(link, target, NULL);
		if(f) UpdateShell(link, false, FI_CREATED);
		return f;
	}
	else if(linkType == LINK_SYMLINK)
	{
		// ���V���{���b�N�����N�͓��ɐ�������.

		// lnhdrlink.exe
		std::wstring path = linkexepath();

		// ����.
		std::vector<wchar_t> vargs(wcslen(link) + wcslen(target) + 10);
		wchar_t* args = &vargs[0];
		_snwprintf(args, vargs.size(), L"\"%ls\" \"%ls\"", link, target);

		// lnhdrlink.exe���s.
		::SHELLEXECUTEINFOW info = {0};
		info.cbSize = sizeof(info);
		info.lpVerb = L"open";
		info.lpFile = path.c_str();
		info.lpParameters = args;
		BOOL f = ::ShellExecuteExW(&info);
		//BOOL f = ::CreateSymbolicLinkW(link, target, isDir ? SYMBOLIC_LINK_FLAG_DIRECTORY : 0);
		if(f) UpdateShell(link, true, FI_CREATED);
		return f;
	}
	else
	{
		return FALSE;
	}
}

BOOL GetLinkTargetPath(wchar_t *buffer, const wchar_t *filename)
{
	// ���p�[�X�o�b�t�@�̊m��
	DWORD dwSize;
	BYTE  reparseBuffer[MAX_REPARSE_SIZE];
	PREPARSE_MOUNTPOINT_DATA_BUFFER data
		= (PREPARSE_MOUNTPOINT_DATA_BUFFER)reparseBuffer;
	HANDLE hFile;

	// �f�t�H���g�ł́A�����Ԃ��Ȃ�
	buffer[0] = 0;

	// ���p�[�X�|�C���g���Ȃ������牽�����Ȃ�
	if(!(GetFileAttributesW(filename) & FILE_ATTRIBUTE_REPARSE_POINT))
		return FALSE;

	// ���p�[�X�|�C���g���J��
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

	// ���擾
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
				// �V���{���b�N�����N
				wchar_t *p
					= (wchar_t *)((BYTE *)&data->PathBuffer
					 + data->TargetNameOffset);
				int len = data->TargetNameLength;

				// "\??\" �Ŏn�܂��Ă�����폜
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

// �R�s�[/�ړ�
bool SymlinkCopyMove(LPCTSTR pszSrcFile, LPCTSTR pszDestFile, bool fMove)
{
	wchar_t buffer[MAX_PATH];
	if(!GetLinkTargetPath(buffer, pszSrcFile))
		return false;

	bool f = (CreateLink(pszDestFile, buffer, LINK_AUTO) != 0);

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
