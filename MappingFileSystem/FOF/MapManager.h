#pragma once
#include "Base.h"

class MapManager {
	HANDLE VMF = 0 ;
	HANDLE MAP = 0 ;
	DWORD PageSize = 64 * 1024;

	size_t FileSize = 0;

	EventCallback * calllback;
	std::string VMFilePath;

public:

	MapManager(EventCallback* Muster) :calllback(Muster) {
		SYSTEM_INFO sysInf;
		GetNativeSystemInfo(&sysInf);
		PageSize = sysInf.dwAllocationGranularity;
		calllback->OnWarring(0," MapProcesser初始化完成", "");
	}

	~MapManager() {
		if (MAP != NULL)CloseHandle(MAP);
		if (VMF != NULL)CloseHandle(VMF);
		
		calllback->OnWarring( 0," MapProcesser退出", "");
	}

	static MapManager* Create(EventCallback* callback,std::string VMFile,size_t size,size_t * completion = NULL) {
		MapManager* mm = new MapManager(callback);
		std::cout << "MM End";
		HANDLE fh = mm->NewFile(VMFile);
		std::cout << "File Handle End";
		CloseHandle(fh);
		if (mm->Open(VMFile)) {
			std::cout << "Open End";
			size_t count = 0;
			while (count < size) {
				count = count + 60 MB;
				std::cout << "ReSize-";
				if (!mm->Resize(count > size ? size : count))return mm;
				if (completion != NULL)*completion = count;
			}
			
			return mm;
		}
		return NULL;
	}

	bool Open(std::string VMFile) {
		VMFilePath = VMFile;

		//读写模式为0 表示只读取信息 在文件被另一个程序占用时才能获得正确的文件大小
		HANDLE fs = OpenFile(VMFilePath, OPEN_EXISTING, 0);
		FileSize = getFileSize(fs);
		CloseHandle(fs);

		VMF = OpenFile(VMFile,OPEN_ALWAYS);
		if (VMF == NULL)return false;
		MAP = Mapping(VMF);

		calllback->OnWarring(0, "打开虚拟文件对象 ", VMFilePath);
		return true;
	}

	void Close() {
		if (MAP != NULL) {
			CloseHandle(MAP);
			MAP = NULL;
		}
		if (VMF != NULL) {
			CloseHandle(VMF);
			VMF = NULL;
		}
		
		calllback->OnWarring(0, "关闭虚拟文件对象 ", VMFilePath);
	}

	size_t getFormatSize() {
		return FileSize;
	}

	static size_t getFileSize(HANDLE File) {
			LARGE_INTEGER size;
			GetFileSizeEx(File, &size);
			return size.QuadPart;
		}

	static size_t getFileSize(std::string FilePath) {
		//读写模式为0 表示只读取信息 后面的语句才能获得正确的文件大小
		HANDLE fs =CreateFile(
			FilePath.c_str(),
			0,//GENERIC_READ | GENERIC_WRITE,//读取权限
			FILE_SHARE_READ,//FILE_SHARE_READ | FILE_SHARE_WRITE,//共享模式 NULL为独占模式
			NULL,//权限
			OPEN_EXISTING,//OPEN_ALWAYS,// 创建模式，打开已有或新建      CREATE_NEW 仅新建，如已经存在则调用失败 OPEN_EXISTING 打开已经存在的文件
			FILE_READ_ATTRIBUTES,//文件属性 FILE_ATTRIBUTE_NORMAL|
			NULL  //复制文件句柄
		);
		size_t size = getFileSize(fs);
		CloseHandle(fs);
		return size;
	}

//////////////////////////////////////////////////////////////////////////
	
	HANDLE OpenFile(std::string Path, DWORD OpenModle= OPEN_EXISTING,DWORD ReadModle= GENERIC_READ | GENERIC_WRITE,DWORD ShareModle= FILE_SHARE_READ | FILE_SHARE_WRITE) {
		SetLastError(0);
		HANDLE VMF= CreateFile(
			Path.c_str(),
			ReadModle,//GENERIC_READ | GENERIC_WRITE,//读取权限
			ShareModle,//FILE_SHARE_READ | FILE_SHARE_WRITE,//共享模式 NULL为独占模式
			NULL,//权限
			OpenModle,//OPEN_ALWAYS,// 创建模式，打开已有或新建      CREATE_NEW 仅新建，如已经存在则调用失败 OPEN_EXISTING 打开已经存在的文件
			FILE_READ_ATTRIBUTES,//文件属性 FILE_ATTRIBUTE_NORMAL|
			NULL  //复制文件句柄
			);
		int error = GetLastError();
		if (error != 0) {
			if (error == 183) {
				calllback->OnWarring(error,"打开文件成功，但文件目标已存在", Path);
			}
			else {
				calllback->OnError(error,"打开文件失败", Path);
				return NULL;
			}
		}
		return VMF;
	}
	
	HANDLE NewFile(std::string Path) { return OpenFile(Path, CREATE_NEW); }
	
	HANDLE Mapping(HANDLE FileHandle, LARGE_INTEGER FileSize = {0}, WCHAR * MappingName = NULL, DWORD MappModle = PAGE_READWRITE) {
		SetLastError(0);
		HANDLE MAP=CreateFileMappingW(
			FileHandle,//文件句柄
			NULL,
			MappModle,//PAGE_READWRITE,
			FileSize.HighPart,
			FileSize.LowPart,
			MappingName
		);
		int error = GetLastError();
		if (error != 0) {
			calllback->OnError(error,"映射文件失败", "");
			return NULL;
		}
		return MAP;
	}
	
	template<typename T = BYTE>
	Shift GetView(SpaceNode node,DWORD ViewModle= FILE_MAP_WRITE | FILE_MAP_READ) {
		size_t size = sizeof(T);
		
		if (node.count*size + node.location > FileSize) {
			std::stringstream ss;
			ss << " 原始大小：" << FileSize <<" 需求大小" << node.count*size + node.location  << std::endl;
			calllback->OnWarring(0, "目标映射段超过文件大小",ss.str());
			throw ERROR_TARGET_SIZE_OVERFLOW;
		}
		
		size_t offset = node.location % PageSize;
		node.location= node.location- offset;

		LARGE_INTEGER position;
		position.QuadPart = node.location;

		SetLastError(0);
		void * ptr=MapViewOfFile(MAP,ViewModle, position.HighPart, position.LowPart,offset + node.count*size);
		int error = GetLastError() ;
		std::stringstream sss;
		
		if (error != 0) {
			sss << " 偏移：" << position.QuadPart << " 长度：" << offset + node.count*size;
			calllback->OnError(error,"内存映射失败", sss.str());
			throw ERROR_VIEW_SEGMENT_OVERFLOW;
		}

		Shift  sp ={ptr,offset};
		
		sss<< " 映射段: " << node.location + offset << " L: " << node.count *size;
		calllback->OnWarring(0,sss.str(),  "");

		return sp;
	}
	 
	bool UnView(void * ptr) {
		return UnmapViewOfFile(ptr);
	}
	bool UnView(Shift ptr) {
		return UnmapViewOfFile(ptr.ptr);
	}

	
//////////////////////////////////////////////////////////////////////////

	bool Resize(LONGLONG newFileSize) {
		LARGE_INTEGER fs;
		fs.QuadPart = newFileSize;
		return Resize(fs);
	}

	//  MSDN：If CreateFileMapping is called to create a file mapping object for hFile,
	//	UnmapViewOfFile must be called first to
	//	unmap all views and call CloseHandle to close the file mapping object
	//	before you can call SetEndOfFile. 
	bool Resize(LARGE_INTEGER newFileSize){
		bool result = true;
		SetLastError(0);
		if (MAP != NULL)CloseHandle(MAP);
		result = result && SetFilePointerEx(VMF, newFileSize, NULL, FILE_BEGIN);
		result = result && SetEndOfFile(VMF);
		result = result && CloseHandle(VMF);
		
		FileSize = getFileSize(VMFilePath);

		VMF = OpenFile(VMFilePath);
		MAP = Mapping(VMF);

		if (!result) {
			calllback->OnError(GetLastError(),"重置文件大小失败", "");
			return false;
		}
		return true;
	}

	bool UpSize(LONGLONG upsize) {
		return Resize(FileSize + upsize);
	}
};


