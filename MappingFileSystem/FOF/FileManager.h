#pragma once
#include"MapManager.h"
#include"HDDProcesser.h"
#include<bitset>
#include<list>
#include<stack>
#include "Messager.h"
#include "NodeProcesser.h"

class FileManager {

	HDDProcesser * hd;
	Messager msg;
	std::string Path;

public:

	FileManager() {
		msg.OnWarring(0, "\n-----------------��ʼ---------------\n", "");
		hd = new HDDProcesser((EventCallback*)&msg);
		msg.OnWarring(0," FileProcesser��ʼ�����", "");
	}

	~FileManager() {
		if(hd!=NULL)delete hd;
		msg.OnWarring(0," FileProcesser�˳�\n-----------------����---------------\n", "");
	}
	

///////////////////////////////�ڵ����///////////////////////////////////////////
	void SaveHead() {
		hd->SaveTreeHead();
	}
	void LoadHead() {
		hd->LoadTreeHead();
	}

	void SaveNodes(PathNode* node) {
		hd->SaveIndexTree(node);
	}
	// recursion  �ݹ�
	void LoadNodes(PathNode* node) {
		hd->LoadIndexTree(node);
	}

	bool Open(std::string path,bool withformat=false,Setting* sets=NULL) {
		Path = path;
		return hd->OpenVMFile(path, withformat,sets);
	}
	void Close() {
		hd->CloseVMFile();
	}
	
	bool MakePartition(std::string name,unsigned char index) {
		return hd->CreatePartition(name,index);
	}

	bool MakeVMFile(std::string path,Setting * sets,size_t * copletion) {
		if (hd->CreateVMFile(path, sets, copletion)) {
			bool result =  hd->OpenVMFile(path, true,sets);
			hd->CloseVMFile();
			return result;
		}
		return false;
	}

	std::string GetVMPath() {
		return Path;
	}
	size_t GetVMSize() {
		return hd->getFileSize(Path);
	}
//////////////////////////////////////////////////////////////////////////

	void RenameNode(std::string newname , PathNode * node) {
		hd->Rename(node ,newname);
	}
	
	//�ڽڵ��´���ָ�����Ƶ��ӽڵ�
	PathNode * CreateFloder(std::string name,PathNode * node) {
		return hd->CreateNewFloder(node, name);
	}

	PathNode * getNodeByPath(std::string Path,PathNode* node) {
		if (node == NULL)return hd->getNodeByPath(Path);
		return hd->getNodeByPath(node,Path);
	}
	
	std::list<PathNode*>  SearchNodeGlobal(std::string name, bool likely = false) {
		return hd->GlobalSearch(name, likely);
	}

	std::list<PathNode*>  SearchNode(std::string name, PathNode* startNode, bool likely = false) {
		return hd->LocalSearch(name, startNode, likely);
	}

	PathNode * GetRoot() {
		return hd->getPartitionRoot();
	}

	void DeleteNode(PathNode* node) {
		if (isBatchFile(node))hd->ReleaseBatchFile(node);
		else if (isNormalFile(node))hd->ReleaseNormalFile(node);
		else if (isHugeFile(node))hd->ReleaseHugeFile(node);
		else if (isFloder(node)) {
			if(node->sub != NULL)for (auto e : *node->sub) {
				DeleteNode(&e);
			}
			//�ļ����Ӷ���ɾ����ϣ�����ɾ����洢���б�index�Ŀռ�
			hd->ReleaseIndexSpace(node);
		}
	}

	void Format(Setting *sets) {
		hd->FormatVMFile(sets);
	}

	PathNode * MakeInnerDir(std::string path) {
		return hd->MakeDirs(path);
	}

	PathNode * MakeInnerDir(std::string path,PathNode* node) {
		return hd->MakeDirs(node, path);
	}

	void ReadFileIn(std::string  outPathName, PathNode * Target, bool AutoRename = false) {
		std::list<std::string> flist;
		flist.push_back(outPathName);
		//int index = outPathName.find_last_of("\\");
		//
		//if (index != outPathName.npos) {
		//	hd->Writer(outPathName.substr(0,index + 1), &flist, Target,AutoRename);
		//}
		//else {
		//	
		//}
		hd->Writer(outPathName, &flist, Target, AutoRename);
	}

	void ReadFileIn(std::string OutBase , std::list<std::string> * FileList, PathNode * Target, bool AutoRename = false) {
		hd->Writer(OutBase, FileList, Target, AutoRename);
	}

	void WriteFileOut(std::string  outPath, PathNode * Source,  bool OverWrite = false) {
		std::list<PathNode*> flist;
		flist.push_back(Source);
		hd->Reader(outPath, &flist , Source->pat, OverWrite);
	}
	void WriteFileOut(std::string  OutPath, std::list<PathNode*> *FileList, PathNode * Parent, bool OverWrite = false) {
		hd->Reader(OutPath, FileList, Parent, OverWrite);
	}

	void CopyFloderIn(std::string   outPath, PathNode* Target, std::string  Filter, bool AutoRename) {
		std::list<std::string> flist;
		//if (!EndWith(outPath, "\\"))outPath += "\\";
		GetFilesUnderFloder(outPath,&flist, Filter,true);
		msg.OnWarring(0,"Copy Floder From ", outPath);
		msg.OnWarring(0,"Copy Floder To ", Target->info.name);
		msg.OnWarring(0,"Copy Floder BasePath ", getParentPath(outPath));
		hd->Writer(getParentPath(outPath), &flist, Target);
	}

	void CopyFloderIn(std::string outPath, std::list<std::string>* OutTarget , PathNode* Target, std::string  Filter, bool AutoRename) {
		std::list<std::string> flist;
		for (auto e : *OutTarget) {
			GetFilesUnderFloder(e, &flist, Filter, true);
		}
		hd->Writer(getParentPath(outPath), &flist, Target, AutoRename);
	}

	void CopyFloderOut(std::string  outPath, PathNode* Source,bool OverWrite) {
		std::list<PathNode*> flist;
		getNodesUnderNode(Source, &flist);
		if (Source->pat != NULL)Source = Source->pat;
		hd->Reader(outPath,&flist,Source, OverWrite);
	}
	
	void CopyFloderOut(std::string  outPath, std::list<PathNode*> * NodeList, PathNode* Source, bool OverWrite) {
		std::list<PathNode*> list;
		for (auto e : *NodeList) {
			getNodesUnderNode(e, &list);
		}
		if (Source->pat != NULL)Source = Source->pat;
		hd->Reader(outPath, &list, Source, OverWrite);
	}

	std::string getParentPath(std::string path) {
		if (EndWith(path, ":\\"))return path;
		else {
			if (EndWith(path, "\\"))path = path.substr(0, path.size() - 1);
			size_t index = path.rfind("\\");
			if (index != path.npos)return path.substr(0, index);
			else return path;
		}
	}
///////////////////////////////////�ļ���д///////////////////////////////////////////

	/*
	�ļ���{δѹ��[ѹ��]} ���ļ���innerCompress outerCompress�� �� Batch�ļ���outerCompress�� �汾
		���ļ�=FILE_TYPE innerCompress  zipSize=ZIP_INNER
		���ļ�=FILE_TYPE outerCompress zipSize>0
		���ļ�=FILE_TYPE withoutCompress zipSize=0
		Batch�ļ�=BATCH_BLOCK|FILE_TYPE outerCompress zipSize>0
		Batch�ļ�=BATCH_BLOCK|FILE_TYPE withoutCompress zipSize=0
	*/
	void _ZipTest(std::string filePath) {
		MapManager mma(&msg);
		HANDLE fh = mma.OpenFile(filePath);
		FileInfo fi = hd->getFileInfo(filePath);

		size_t dsize = 0;
		size_t odata = fi.data.count;

		void* Buffer = malloc(odata);
		void* ZBuffer = NULL;
		
		ReadFile(fh, Buffer, odata, NULL, NULL);
		hd->ZipData(Buffer,fi.data.count,&ZBuffer,&dsize);
		hd->UnZipData(ZBuffer, dsize, &Buffer, &odata);
	}
}; 