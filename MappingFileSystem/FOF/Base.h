#pragma once
#include<windows.h>
#include<list>
#include<iostream>
#include<ctime>
#include<sstream>
#include<fstream>
#include<string>
#include<zconf.h>
#include<zlib.h>



#define KB * 1024
#define MB KB KB
#define GB MB KB
#define TB GB KB
#define PB TB KB

////////////////////////////////Error Code//////////////////////////////////////////
#define ERROR_HOLD_SPACE -1
#define ERROR_VIEW_SEGMENT_OVERFLOW -2
#define ERROR_TARGET_SIZE_OVERFLOW -3

//////////////////////////////////////////////////////////////////////////
//Index 节点状态使用
//Type  节点属性
//Block 系统节点附加属性
#define INDEX_NORMAL 0
#define INDEX_NAME_CHANGE 1
#define INDEX_SUB_MODIFY 1<<1
#define INDEX_SELF_MODIFY 1<<2
#define INDEX_SUB_SIZECHANGED 1<<3
#define INDEX_BATCH_DATA_CHANGED 1<<4

#define ROOT_TYPE      1<<5
#define PARTITION_TYPE 1<<6
#define FLODER_TYPE    1<<7
#define FILE_TYPE      1<<8
#define BATCH_TYPE     1<<9
#define HUGE_TYPE      1<<10
#define NORMAL_TYPE    1<<11
#define BATCH_BLOCK    1<<12
#define LONG_NAME      1<<13

//节点名称最长长度
//#define MAX_NAME_SIZE 
#define NAME_SIZE 128
//Batch节点的块偏移列表起始位置与大小
#define SPL_SIZE  ((NAME_SIZE * sizeof(char))/sizeof(LONGLONG))-1
#define SPL_START SPL_SIZE-1

//超长节点名的存储位置及大小(Batch节点的名称不会超过24个【目前来看】，所以不会冲突)
#define LN_SIZE   SPL_SIZE
#define LN_START  SPL_START
//内存地址，指向内存中的长文件名
#define LN_PTR    LN_START-1

#define FORMAT_SYMBOL 5
#define FORMAT_VERSION 0.2333f





struct Setting {
	const unsigned int Mark = FORMAT_SYMBOL;
	float Version = FORMAT_VERSION;
	bool Compressable = true;
	LONGLONG VMSize = 256 MB;
	DWORD BatchBlockSize = 48 MB;//小文件连续存储块最大大小 
	DWORD SectionSize = 64 MB; //单次最大映射段大小 上限为磁盘一秒读写量
	DWORD HugeFileSize = 128 MB;//判定为大文件的尺寸限制，将影响存储决策
};

/************************************************************************/
/* Location Count Status                                                */
/************************************************************************/
typedef struct Node {
	LONGLONG location = 0;
	LONGLONG count = 0;//数据大小 或是 子项个数
	LONGLONG blockindex = -1; //标记所属 batch块 或是 sequence块
}SpaceNode,ZipNode;

/************************************************************************/
/* Point Offset                                                         */
/************************************************************************/
struct Shift {
	void * ptr = NULL;
	size_t offset = 0;

	BYTE* getEntry() { return (BYTE*)ptr + offset; };
};

/************************************************************************/
/* Bits CreateTime AccessTime ModifyTime Name SpaceNode                 */
/************************************************************************/
struct FileInfo {
	ULONG type = 0;
	ULONGLONG c_time = 0;
	ULONGLONG a_time = 0;
	ULONGLONG m_time = 0;
	CHAR name[NAME_SIZE] = {'\0'};
	SpaceNode data; //若是文件 则为实际数据存储位置； 若为文件夹 则为子文件列表
	LONG zipedSize = 0;

	bool setName(std::string str_name) {
		if (str_name.size()+1 > NAME_SIZE) {
			//生成短名称
			str_name = str_name.substr(0,NAME_SIZE - 3*sizeof(LONGLONG));
			return false;
		}
		memcpy(name, str_name.c_str(), str_name.length()+1);
		type |= INDEX_SELF_MODIFY;
		return true;
	}
};

struct ProcessNode {
	std::string Outer;
	FileInfo Info;
};

//文件树
struct PathNode {
	FileInfo info;
	std::list<PathNode> * sub =NULL;
	PathNode * pat = NULL;

	void addChild(PathNode* node) {
		if (sub == NULL)sub = new std::list<PathNode>();
		node->pat = this;
		sub->push_back(*node);
		info.type |= INDEX_SUB_SIZECHANGED;
	}
	void removeChild(PathNode* node) {
		if (sub == NULL)return;
		sub->remove(*node);
		info.type |= INDEX_SUB_SIZECHANGED;
	}
	bool operator ==(const  PathNode& p) {
		std::string mm = this->info.name;
		bool ns = mm.compare(p.info.name)==0;
		bool nt = this->info.type == p.info.type;
		if (ns && nt && this->pat == p.pat)return true;
		return false;
	}
};
//
//struct Head {
//	SpaceNode TotalSize;//虚拟文件大小
//	PathNode Root;
//	SpaceNode EmptyZone;//可用空间列表
//};
struct SystemHead
{
	ULONGLONG FreeSize = 0;
	Setting Sets;
	PathNode Root;
	SpaceNode EmptySpaces;
};

struct MemoryNode {//内存输出标识节点
	void * ptr;
	std::string name;
	size_t offset;
	size_t size;
};

//字符处理
static bool StartWith(std::string src,std::string cmp,size_t Offset =0 ) {
	size_t index = src.find(cmp);
	if (index == src.npos)return false;
	else return index == Offset ? true : false;
}
static bool EndWith(std::string src, std::string cmp) {
	return src.rfind(cmp) == src.length()- cmp.length() ? true : false;
}
static std::list<std::string> Split(std::string str,std::string spliter) {
	size_t index = 0;
	std::list<std::string> list;
	index = str.find_first_of(spliter);
	while (index != str.npos) {
		list.push_back(str.substr(0, index));
		str = str.substr(index + 1);
		index = str.find_first_of(spliter);
	}
	if (str.size() > 0)list.push_back(str);
	return list;
}
static std::string  ReplaceAll(std::string str, std::string replacer,std::string target="") {
	size_t index = str.find(replacer);
	while (index != str.npos) {
		str = str.replace(index, replacer.size(), target);
	}
	return str;
}

//功能启用
static bool getBool(std::string str) {
	std::stringstream strm;
	strm << std ::boolalpha << str;
	bool sw;
	strm >> sw;
	return sw;
}
//空间大小
static size_t getSize(std::string str) {
	std::stringstream stm;
	double size = 0;
	if (EndWith(str, "KB")) {
		stm << str.substr(0, str.find("KB"));
		stm >> size;
		return (size_t)size KB;
	}
	if (EndWith(str, "MB")) {
		stm << str.substr(0, str.find("MB"));
		stm >> size;
		return (size_t)size MB;
	}
	if (EndWith(str, "GB")) {
		stm << str.substr(0, str.find("GB"));
		stm >> size;
		return (size_t)size GB;
	}
	if (EndWith(str, "TB")) {
		stm << str.substr(0, str.find("TB"));
		stm >> size;
		return (size_t)size TB;
	}
	if (EndWith(str, "PB")) {
		stm << str.substr(0, str.find("PB"));
		stm >> size;
		return (size_t)size PB;
	}
	return (size_t)size;
}
static std::string ConvertSpaceToRead(size_t size) {
	size_t F, E;
	F = size / (1 MB);
	E = size % (1 MB);
	std::stringstream ss;
	ss << F << "." << E << "MB";
	return ss.str();
}
static std::string ConvertBool(bool sw) {
	std::stringstream ss;
	ss << std::boolalpha << sw;
	return ss.str();
}
static std::string ConvertSize(size_t sz) {
	std::stringstream ss;
	ss << sz;
	return ss.str();
}

//时间
static std::string getTimeToString(ULONGLONG time) {
	tm ts;
	time_t tt = time;
	localtime_s(&ts, &tt);
	std::stringstream sst;
	sst << ts.tm_year + 1900 << "年" << ts.tm_mon + 1 << "月" << ts.tm_mday << "日" << ts.tm_hour << ":" << ts.tm_min << ":" << ts.tm_sec;
	return sst.str();
}

static std::string getTimeToString() {
	tm  ts ;
	time_t tt = time(0);
	localtime_s(&ts,&tt);
	std::stringstream sst;
	sst << ts.tm_year + 1900 << "年" << ts.tm_mon + 1 << "月" << ts.tm_mday << "日" << ts.tm_hour << ":" << ts.tm_min << ":" << ts.tm_sec;
	return sst.str();
}
FILETIME TimetToFileTime(time_t t)
{
	FILETIME ft;
	LONGLONG ll = Int32x32To64(t, 10000000) + 116444736000000000;
	ft.dwLowDateTime = (DWORD)ll;
	ft.dwHighDateTime = (DWORD)(ll >> 32);
	return ft;
	//FileTimeToSystemTime(&ft, pst);
}
std::string ConvertToString(LONGLONG data) {
	std::stringstream ss;
	ss << data;
	return ss.str();
}

template<typename T=PathNode>
T * at(size_t index, std::list<T> * list) {
	if (list == NULL || list->size() == 0 || index >= list->size())return NULL;
	size_t s = 0;
	for (typename std::list<T>::iterator e = list->begin(); e != list->end(); e++) {
		if (s == index)return &(*e);
		s++;
	}
	return NULL;
}

template<typename T=PathNode>
auto eraserat(size_t index, std::list<T> * list, PathNode * DeletedNode = NULL) {
	typename std::list<T>::iterator e = list->begin();
	size_t s = 0;
	for (; e != list->end(); e++) {
		if (s == index) {
			if(DeletedNode!=NULL)*DeletedNode = *e;
			break;
		}
		else s++;
	}
	return list->erase(e);
}


//事件回调
class EventCallback {

public:
	virtual ~EventCallback() {};
	virtual void OnEventFinish(int status, std::string Exinfo) = 0;
	virtual void OnError(int ErrorCode,std::string Msg, std::string Exinfo ) = 0;
	virtual void OnWarring(int WarringCode,std::string Msg, std::string Exinfo) = 0;
};


class FileReader
{
private:
	std::ifstream * file;
	std::string line;
public:
	~FileReader() {
		if (file != NULL) {
			if (file->is_open())file->close();
			delete file;
		}
		line.clear();
	}
	bool Open(char* filepath, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::binary) {
		file = new std::ifstream(filepath, mode);
		if (file)return true;
		return false;
	}
	bool ReadLine(std::string * src) {
		if (std::getline(*file, line)) {
			src->swap(line);
			return true;
		}

		else return false;
	}

	template<typename T>
	T *  ReadData(size_t  length) {
		T * buffer = (T *)malloc(length * sizeof(T));
		if (file->read((char *)buffer, length * sizeof(T)))return buffer;
		return NULL;
	}

	template<typename T>
	T * ReadData(T ** buffer, size_t  length) {
		*buffer = (T*)malloc(length * sizeof(T));
		if (file->read((char *)*buffer, length * sizeof(T)))return *buffer;
		return NULL;
	}

	void Close() {
		file->close();
	}

};

class FileWriter {
	std::ofstream * file;

	bool _WriteEnter() {
		if (file->write("\n", 1))return true;
		return false;
	}
public:

	~FileWriter()
	{
		if (file != NULL) {
			if (file->is_open())file->close();
			delete file;
		}
	};
	bool Open(char * filepath, std::ios_base::openmode mode = std::ios_base::out | std::ios_base::binary ) {
		std::string path = filepath;
		//path.append("\\").append(getTime()).append(".txt");
		//FileProcess::createFolders(path, path);

		//setlocale(LC_ALL, "Chinese-simplified");
		file = new std::ofstream((char*)path.c_str(), mode);
		bool result = file->is_open();
		//setlocale(LC_ALL, "C");

		if (result)return true;

		std::cout << " Open File Error " << std::endl;
		return false;
	}
	void WriteLine(std::string * line) {
		//FileWriteLocker.lock();
		file->write(line->c_str(), line->size());
		_WriteEnter();
		//FileWriteLocker.unlock();
		file->flush();
	}

	template<typename T>
	bool WriteData(T * data, size_t length) {
		if (!file->write((char *)(data), sizeof(T)*length)) {
			return false;
		}

		return true;
	}

	void Close() {
		file->close();
	}
};