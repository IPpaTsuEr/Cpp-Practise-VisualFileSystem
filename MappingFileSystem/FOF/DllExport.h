#pragma once
#include "MapManager.h"
#include "HDDProcesser.h"
#include "Messager.h"
#include "FileManager.h"

#define DLL_EXPORT

#ifdef DLL_EXPORT
#define DLL_API extern "C" _declspec(dllexport)
#define Call _stdcall
#else
#define DLL_API 
#endif 
/************************************************************************/
/*DLL 函数 导出*/
/************************************************************************/
FileManager * G_instance = NULL;


std::list<std::string>* FileInqueue;
std::list<std::string>* FloderInqueue;
std::list<PathNode*>* FileOutqueue;
std::list<PathNode*>* FloderOutqueue;
/************************************************************************/
/* 
地址栏输入路径进行跳转 --> node* geNodeFromAbsolutePath（string abpath）
从node直接进入下一级 --> nodes* node->sub（）
从node返回上一级 --> node* node->sub（）
新建节点 --> node* newnode（string name）
重命名节点--> void rename（string name）
删除节点 --> bool delete（node *）
创建路径 --> node* makedirs（string abpath）

读写： 写入，写出（批量、单独、内存）【压缩/不压缩】

查找： 全局查找 指定文件夹下查找

系统： 实例化对象，格式化空间，载入节点，保存节点

转换： 绝对路径，相对路径 转换 节点
	   数字时间 转换 字符节点

释放： 实例对象释放（单点），数组指针释放（一维）

*/
/************************************************************************/
struct GroupPoit {
	void ** ptr;
	size_t size;
};

template<typename T = PathNode * >
static void** ListToPoints(std::list<T> * src, size_t * size) {
	*size = src->size();
	void** list = (void**)malloc(*size * sizeof(void*));
	size_t index = 0;
	for (auto i = src->begin(); i != src->end(); i++) {
		T tem = *i;
		list[index] = tem;
		index++;
	}
	return list;
}
DLL_API bool Call IsPartition(PathNode * node) {
	return isPartition(node);
}
DLL_API bool Call IsFile(PathNode * node) {
	return isFile(node);
}
DLL_API bool Call IsRoot(PathNode * node) {
	return isRoot(node);
}
DLL_API void Call ReleaseGropuPoitPtr(GroupPoit * GP) {
	free(GP->ptr);
	delete GP;
}
DLL_API void Call ReleaseSinglePtr(void * ptr) {
	free(ptr);
}
static bool IsInited = false;

DLL_API  void Call InitFileManagerInstance() {
	G_instance = new FileManager();
	if (G_instance != NULL)IsInited = true;
	FileInqueue = new std::list<std::string>();
	FloderInqueue = new std::list<std::string>();
	FileOutqueue = new std::list<PathNode *>();
	FloderOutqueue = new std::list<PathNode *>();
}

DLL_API void Call ReleaseFileManagerInstance() {
	IsInited = false;
	delete G_instance;
	delete FileInqueue;
	delete  FloderInqueue;
	delete  FileOutqueue;
	delete  FloderOutqueue;
}
DLL_API bool Call Inited() {
	return IsInited;
}

DLL_API PathNode* Call GetNodeByPath(char * Path, PathNode* node) {
	return  G_instance->getNodeByPath(Path,node);
}

DLL_API PathNode* Call MakeInnerDirViaAbsolutePath(char* AbsolutePath) {
	return G_instance->MakeInnerDir(AbsolutePath);
}

DLL_API PathNode* Call MakeInnerDirViaRelativePath(char* RelativePath, PathNode* node) {
	return G_instance->MakeInnerDir(RelativePath, node);
}


DLL_API GroupPoit * Call Enter(PathNode* Current) {
	GroupPoit * gp = new GroupPoit();
	if (Current->sub == NULL || Current->sub->size() == 0) {
		gp->ptr = NULL;
		gp->size = 0;
		return  gp;
	}
	std::list<PathNode*> list;
	size_t size;
	for (auto e = Current->sub->begin(); e != Current->sub->end(); e++) {
		if(!isBatch(&(*e)))list.push_back(&(*e));
	}
	
	gp->ptr=ListToPoints(&list, &size);
	gp->size = size;
	return  gp;
}

DLL_API GroupPoit * Call Back(PathNode* Current) {

	if (isRoot(Current) || Current->pat == NULL) {
		//MessageBoxA(NULL, " Parent is null", "NULL", 0);
		return Enter(Current);
	}
	else {
		//MessageBoxA(NULL, Current->pat->info.name, " Back to", 0);
		return Enter(Current->pat); 
	}
}

DLL_API GroupPoit * Call JumpTo(char * Path,PathNode * node) {
	PathNode * TargetNode = GetNodeByPath(Path, node);
	if (TargetNode == NULL) return NULL;
	return Enter(TargetNode);
}

DLL_API PathNode* Call GetRootNode() {
	return G_instance->GetRoot();
}
DLL_API PathNode* Call GetParentNode(PathNode* node) {
	if (node->pat == NULL || isRoot(node))return node;
	return node->pat;
}

DLL_API bool Call NewPartition(char * name, unsigned char index) {
	return G_instance->MakePartition(name, index);
}
DLL_API void Call NewFloder(char* name, PathNode * SelectedNode) {
	//MessageBoxA(NULL, SelectedNode->info.name, "new Floder", 0);
	G_instance->CreateFloder(name, SelectedNode);
}

static size_t Percent = 0;

DLL_API bool Call NewVMFile(char* outpath,Setting sets) {
	Percent = 0;
	Setting st = sets;
	return G_instance->MakeVMFile(outpath,&st, &Percent);
}
DLL_API int Call GetCreateNewVMFileProcess() {
	return Percent;
}

DLL_API char* Call GetVMPath() {

	return (char*) G_instance->GetVMPath().c_str();
}
DLL_API size_t Call GetVMSize() {
	return G_instance->GetVMSize();
}

DLL_API void Call DleteNode(PathNode * SelectedNode) {
	G_instance->DeleteNode(SelectedNode);
}
DLL_API void Call RenameNode(char* newName, PathNode * SelectedNode) {
	G_instance->RenameNode(newName, SelectedNode);
}

DLL_API void Call Format(Setting sets) {
	G_instance->Format(&sets);
}
DLL_API bool Call OpenVM(char * path,bool withformat,Setting sets) {
	return G_instance->Open(path, withformat,&sets);
}
DLL_API void Call CloseVM() {
	G_instance->Close();
}


DLL_API char * Call GetNodeType(PathNode * node) {
	std::string str = getFileType(node);
	char* type = (char*)malloc(sizeof(char)*str.size() + 1);
	memcpy(type, str.c_str(), str.size() + 1);
	return type;
}
DLL_API char * Call ConvertTime(size_t time) {
	std::string t = getTimeToString(time);
	char* stringTime =(char*) malloc(sizeof(char)*t.size() + 1);
	memcpy(stringTime, t.c_str(), t.size() + 1);
	return stringTime;
}

DLL_API GroupPoit * Call Search(char* name, PathNode * node, bool likely = false) {
	//std::cout << "=======" << name << "    " << node->info.name << std::ios_base::boolalpha << likely << std::endl;

	std::list<PathNode*> result;
	if(node==NULL)result = G_instance->SearchNode(name, node, likely);
	else result = G_instance->SearchNodeGlobal(name, likely);

	//std::cout << "=======" << result.size() << std::endl;
	size_t size = 0;
	GroupPoit * gp = new GroupPoit();
	gp->ptr = ListToPoints(&result, &size);
	gp->size = size;
	return  gp;
}


DLL_API void Call CopyFileIn(char*  outPathName, PathNode * Target, bool AutoRename) {
	G_instance->ReadFileIn(outPathName, Target, AutoRename);
}
DLL_API void Call CopyFileOut(char*  outPath, PathNode * Source, bool OverWrite) {
	G_instance->WriteFileOut(outPath, Source, OverWrite);
}

DLL_API void Call CopyFloderIn(char*  outPath, PathNode* Target, char*  Filter ,bool Autorename) {
	G_instance->CopyFloderIn(outPath, Target,  Filter, Autorename);
}
DLL_API void Call CopyFloderOut(char*  outPath, PathNode* Source, bool OverWrite) {
	G_instance->CopyFloderOut(outPath, Source, OverWrite);
}


DLL_API void SendToInQueue(char * path, bool isFile) {
	if (isFile)FileInqueue->push_back(path);
	else FloderInqueue->push_back(path);
}
DLL_API void SendToOutQueue(PathNode * node, bool isFile) {
	if (isFile)FileOutqueue->push_back(node);
	else FloderOutqueue->push_back(node);
}

DLL_API void Call SubmitIn(char * OutBase,PathNode * ParentNode,char * Filter, bool Rename) {
	G_instance->CopyFloderIn(OutBase,FloderInqueue, ParentNode,Filter,Rename);
	G_instance->ReadFileIn(OutBase, FileInqueue, ParentNode, Rename);

	FileInqueue->clear();
	FloderInqueue->clear();
}
DLL_API void Call SubmitOut(char * OutPath,PathNode * ParentNode,bool Overwrite) {
	G_instance->CopyFloderOut(OutPath,FloderOutqueue,ParentNode,Overwrite);
	G_instance->WriteFileOut(OutPath, FileOutqueue,ParentNode,Overwrite);
	
	FloderOutqueue->clear();
	FileOutqueue->clear();
}
