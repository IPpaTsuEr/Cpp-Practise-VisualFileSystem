#pragma once
#include"Base.h"

#include<io.h>
#include <direct.h>
#include "Setter.h"


//////////////////////////////////�ж�����////////////////////////////////////////
	static bool isRoot(PathNode *node) {
		//std::cout <<" Type "<<node->info.type <<" Root "<< ROOT_TYPE <<"  "<< node->info.name  << " &=>"<< (bool)(node->info.type & ROOT_TYPE) <<std::endl;
		return  (node->info.type & ROOT_TYPE);
	}
	static bool isPartition(PathNode * node) {
		return node->info.type & PARTITION_TYPE;
	}
	static bool isFloder(PathNode * node) {
		return node->info.type & FLODER_TYPE;
	}
	static bool isBatch(PathNode * node) {
		return node->info.type & BATCH_TYPE && node->info.type & FLODER_TYPE;
	}
	static bool isFile(PathNode * node) {
		return node->info.type & FILE_TYPE;
	}
	static bool isBatchBlock(PathNode * node) {
		return node->info.type & BATCH_BLOCK && node->info.type & FILE_TYPE;
	}
	static bool isLongName(PathNode * node) {
		return node->info.type & LONG_NAME;
	}

	static bool isHugeFile(PathNode* node) {
		return node->info.type & HUGE_TYPE && node->info.type & FILE_TYPE;
	}
	static bool isNormalFile(PathNode * node) {
		return node->info.type & NORMAL_TYPE && node->info.type & FILE_TYPE;
	}
	static bool isBatchFile(PathNode* node) {
		return node->info.type & BATCH_TYPE && node->info.type & FILE_TYPE;
	}

	static bool isSubGetable(PathNode * node) {
		return isRoot(node) || isPartition(node) || isFloder(node) || isBatch(node);
	}

/////////////////////////////////////�ڵ����/////////////////////////////////////

		//��ȡ�ڵ������нڵ㣨�ļ�����ڵ� ���� batch��sequence��ʽд����ļ��ڵ㣩
	static void getNodesUnderNode(PathNode * node, std::list<PathNode *> * list) {
		if (isHugeFile(node) || isBatchFile(node) || isNormalFile(node)) {
			list->push_back(node);
			//std::cout << "�ڽڵ� \t" << node->pat->info.name << "\t ȡ�� " << node->info.name << std::endl;
		}
		if (isFloder(node)) {
			if (node->sub != NULL && node->sub->size() > 0) {
				for (auto e = node->sub->begin(); e != node->sub->end(); e++) {
					//std::cout << "����ڵ� \t" << e->info.name << std::endl;
					getNodesUnderNode(&(*e), list);
				}
			}
		}
	}
	//ͨ���ڵ������ҵ��ڵ�(���޽ڵ�ĳ����ӽڵ�)
	static PathNode * FindNodesInFloder(PathNode * node, std::string name) {
		if (node->sub == NULL)return NULL;
		std::list<PathNode>::iterator it = node->sub->begin();
		while (it != node->sub->end())
		{
			if (isLongName(node)) {
				LONGLONG * ptr = (LONGLONG*)node->info.name;
				std::string * ln = (std::string *)ptr[LN_PTR];
				if (ln != NULL && ln->compare(name) == 0)node;
			}
			else if (name.compare(it->info.name) == 0)return &(*it);
			it++;
		}
		return NULL;
	}
	//ָ���ڵ����Ƿ����ָ���ļ�Ŀ��
	static bool IsFileNodeExist(std::string filename, PathNode * parent_node) {
		if (parent_node->sub == NULL || parent_node->sub->size() == 0)return false;
		if (FindNodesInFloder(parent_node, filename) != NULL)return true;
		return false;
	}

//////////////////////////////////////////////////////////////////////////
	static void setFileInfo(HANDLE file, FileInfo info) {
		FILETIME cft, mft, aft;
		aft = TimetToFileTime(info.a_time);
		mft = TimetToFileTime(info.m_time);
		cft = TimetToFileTime(info.c_time);
		SetFileTime(file, &cft, &aft, &mft);
	}

	static std::string getFileType(PathNode * node) {
		if (isFile(node)) {
			std::string fileName = node->info.name;
			size_t DotIndex = fileName.find_last_of(".");
			if (DotIndex != fileName.npos) {
				return fileName.substr(DotIndex);
			}
			return "Floder";
		}
		if (isPartition(node))return "Disk";
		if (isRoot(node))return "Root";
		return "Floder";
	}
	//�����ļ�����������չ����������չ������Unknown
	static std::string getFileType(std::string fileName) {
		size_t DotIndex = fileName.find_last_of(".");
		if (DotIndex != fileName.npos) {
			return fileName.substr(DotIndex);
		}
		return "Unknown";
	}

	//��ȡ���ڵ��ڸ��ڵ��е����
	static long long getIndexInParent(PathNode * node) {
		std::list<PathNode>::iterator it = node->pat->sub->begin();
		long long index = 0;
		while (it != node->pat->sub->end()) {
			if (strcmp(it->info.name, node->info.name) == 0)return index;
			index++;
			it++;
		}
		return -1;
	}

	//��ȡ�ⲿ�ļ�·���������ļ�
	static void GetFilesUnderFloder(std::string outPath, std::list<std::string> * result, std::string filter = "*", bool includeSub = false) {
		_finddata_t data;

		std::string name = outPath;
		if (!EndWith(name, "\\*")) {
			if (!EndWith(name, "\\"))name.append("\\");
			if (!EndWith(name, "*"))name.append("*");
		}

		intptr_t handle = _findfirst(name.c_str(), &data);
		do {
			if (strcmp(data.name, ".") == 0 || strcmp(data.name, "..") == 0 || data.attrib & _A_SYSTEM)continue;
			//���ȴ����ļ���ֹ�ݹ����
			if (!(data.attrib & _A_SUBDIR)) {//���ļ�
				std::string fn = data.name;
				size_t ind = fn.find_last_of(".");

				if (filter.empty() || filter.compare("*") == 0) {//������Ϊ�����ļ�
					goto SAVEPOINT;
				}
				else {
					if (ind != fn.npos) {

						if (filter.find(fn.substr(ind)) != filter.npos) {//ͨ��ɸѡ��ɸѡ
						//ʹ�þ���·��
						SAVEPOINT:
							std::string savepath = outPath;
							if (EndWith(savepath, "\\"))savepath.append(data.name);
							else savepath.append("\\").append(data.name);

							result->push_back(savepath);
						}
					}
				}
			}
			else {//���ļ��� 
				if (includeSub) {
					std::string entry = outPath;
					if (!EndWith(entry, "\\"))entry.append("\\");
					entry.append(data.name);
					GetFilesUnderFloder(entry, result, filter, true);
				}
			}
		} while (0 == _findnext(handle, &data));
		_findclose(handle);
	}


/////////////////////////////////////·������/////////////////////////////////////
	//���ļ�·������ȡ·��(����\��β������Ϊ·�������ļ���)
	static std::string getPathFromFilePath(std::string FilePath) {
		size_t d_i = FilePath.rfind(".");
		size_t p_i = FilePath.rfind("\\");
		std::string result;
		if (d_i == FilePath.npos) {
			if (p_i == FilePath.npos)return FilePath;
			else result = FilePath.substr(0, p_i+1);
		}
		if (d_i > p_i)result = FilePath.substr(0, p_i+1);
		else result = FilePath;

		if (EndWith(result, "\\") && !EndWith(result, ":\\"))result = result.substr(0,result.size()-1);
		if (StartWith(result, "\\"))result = result.substr(1);

		return result;
	}

	// ���� C:/a/p.ocx �ᱻ�ж�Ϊ�ļ�·����C:/a/p.ocx/ ���ж�Ϊ�ļ���·�� �ᴴ������׺�����ļ���
	static int MakeOutDirs(std::string AbsolutePath, std::string & original) {
		/*
		����ѹջ
		�� C:/a/b/c
		�ں����ݹ�ջ��˳��Ϊ
		��ջ�ס�C:/a/b/c
				C:/a/b
				C:/a
		��ջ����C:
		Ȼ�� ���γ�ջ������Ƿ���ڣ��������򴴽�
		*/
		size_t location = AbsolutePath.find_last_of("\\");

		if (location != AbsolutePath.npos) {
			//������·�����ݹ�
			std::string sortpath = AbsolutePath.substr(0, location);
			//if (AbsolutePath.size() > sortpath.size())
			MakeOutDirs(sortpath, original);
			
		}
		if (AbsolutePath.compare(original) == 0) {
			if (EndWith(AbsolutePath,"\\")) {
				return _mkdir(AbsolutePath.c_str());
			}
			else {
				return 0;
			}
		}
		//�Ƿ����
		if (0 != _access(AbsolutePath.c_str(), 0)) {
			return _mkdir(AbsolutePath.c_str());
		}
		else {
			return 0;
		}

	}

	static bool IsOutFileExist(std::string path) {
		DWORD attr = GetFileAttributes(path.c_str());
		return INVALID_FILE_ATTRIBUTES != attr;
	}

	static bool AutoRename(FileInfo * fn, PathNode * parentNode) {
		//std::stringstream s_name;
		//std::string name = fn->name;
		//size_t index = 0, sequence = 0;

		//while (IsFileNodeExist(name, parentNode)) {//�����ļ�����

		//	name = fn->name;//���õ�ԭ�ļ���
		//	index = name.find_last_of(".");//�Ƿ��к�׺��
		//	if (index != name.npos && fn->type & FILE_TYPE) {//�ں�׺��ǰ�������
		//		s_name << name.substr(0, index) << "_" << sequence << name.substr(index);
		//	}
		//	else {//���ļ��л��޺�׺�����ļ�ֱ�Ӳ������ 
		//		s_name << name << "_" << sequence;

		//	}
		//	name = s_name.str();//�����´β���
		//	s_name.clear();
		//	s_name.str("");
		//	sequence++;//��һ�β�������
		//}

		//if (name.compare(fn->name) != 0) {
		//	memcpy(fn->name, name.c_str(), name.size());
		//	return true;
		//}
		//return false;
		std::stringstream s_name;
		std::string name, oname;
		size_t index = 0, sequence = 0;

		if (fn->type & LONG_NAME) {
			std::string * ln = new std::string(name);
			LONGLONG * ptr = (LONGLONG*)fn->name;
			name = ((std::string *)ptr[LN_PTR])->c_str();
			oname = name;
		}
		else {
			name = fn->name;
			oname = name;
		}

		while (IsFileNodeExist(name, parentNode)) {//�����ļ�����

			name = oname;//���õ�ԭ�ļ���
			index = name.find_last_of(".");//�Ƿ��к�׺��
			if (index != name.npos && (fn->type & FILE_TYPE)) {//�ں�׺��ǰ�������
				s_name << name.substr(0, index) << "_" << sequence << name.substr(index);
			}
			else {//���ļ��л��޺�׺�����ļ�ֱ�Ӳ������ 
				s_name << name << "_" << sequence;

			}
			name = s_name.str();//�����´β���
			s_name.clear();
			s_name.str("");
			sequence++;//��һ�β�������

		}

		if (name.compare(oname) != 0) {
			if (!fn->setName(name)) {
				std::string * ln = new std::string(name);
				LONGLONG * ptr = (LONGLONG*)fn->name;
				std::string * sptr = (std::string *)ptr[LN_PTR];
				if (sptr != NULL)delete sptr;
				ptr[LN_PTR] = (LONGLONG)ln;
				fn->type |= INDEX_NAME_CHANGE;
			}
			return true;
		}
		return false;
	}

	static bool AutoRename(PathNode * fn, PathNode * parentNode) {
		return AutoRename(&fn->info, parentNode);
	}
	
/////////////////////////////·��/////////////////////////////////////////////
	static void getAbsoulutePath(PathNode * node, std::string * result) {
			if (isRoot(node)) {
				//result->insert(0, "Root@");
			}
			else if (!isPartition(node)) {
				result->insert(0, std::string().append("\\").append(node->info.name));
				getAbsoulutePath(node->pat, result);
			}
			else {
				result->insert(0, std::string().append((char*)&node->info.a_time).append(":"));
			}
		}

		static std::string getAbsoulutePath(PathNode * node) {
			std::string path;
			getAbsoulutePath(node, &path);
			return path;
		}

	static std::string getRelativePath(std::string BasePath, std::string FullPath, std::string replacer = "") {
		if (FullPath.at(0) == BasePath.at(0)) {
		//ͬ�����£���ͨ����Զ�λ
			//�������ϵ
			if (StartWith(FullPath, BasePath)) {
				FullPath.replace(0, BasePath.size(), replacer);
				return FullPath;
			}
			//���ְ�����ϵ
			std::list<std::string> BL =Split(BasePath, "\\");
			std::list<std::string> TL = Split(FullPath, "\\");
			auto B = BL.begin();
			auto T = TL.begin();
			size_t index = 0;
			std::string same;
			while (B!=BL.end() && B->compare(*T) == 0) {
				index++;
				same.append(*B).append("\\");
				B++; T++;
			}
			std::string result;
			while (index > 0) {
				result.append(".\\");
				index--;
			}
			return FullPath.replace(0, same.size(), result);
		}
		else {
		//�������ֱ��ͨ�����Զ�λ
			return FullPath;
		}
		
	}

	static std::string  getRelativePath(PathNode * RootNode, PathNode * TargetNode) {
		//std::string path;
		//while (TargetNode != RootNode) {
		//	if (isPartition(TargetNode)) {
		//		path.insert(0, std::string().append((char*)&TargetNode->info.a_time).append(":"));
		//		break;
		//	}
		//	path.insert(0, std::string().append("\\").append(TargetNode->info.name));
		//	TargetNode = TargetNode->pat;
		//}
		//return path;
		std::string rpath = getAbsoulutePath(RootNode);
		std::string tpath = getAbsoulutePath(TargetNode);
		std::string result = getRelativePath(rpath, tpath);
		if (EndWith(result, "\\") && !EndWith(result, ":\\"))result = result.substr(0, result.size() - 1);
		if (StartWith(result, "\\"))result = result.substr(1);

		return result;
	}

	
////////////////////////////////�����ļ�//////////////////////////////////////////

	static void LoadSetting(EventCallback * callback, Setter * Sets) {
		FileReader fr;
		if (!fr.Open((char*)".\\setting.ini")) {
			callback->OnError(GetLastError()," FileReader '.\\setting.ini' ",  "");
		}
		std::string line;
		while (fr.ReadLine(&line)) {
			if (StartWith(line, "#")) {
				//ע�ͱ�� ������

			}
			else {
				line = ReplaceAll(line, " ");
				line = ReplaceAll(line, ";");
				std::list<std::string> kv = Split(line, "=");
				if (kv.size() > 2) {
					callback->OnError(0, "������Ϣ�����쳣��ͬһ���������˶��ֵ", line);
				}
				else if (kv.size()==2) {
					if (EndWith(kv.back(), "KB") || EndWith(kv.back(), "MB") || EndWith(kv.back(), "GB") || EndWith(kv.back(), "TB") || EndWith(kv.back(), "PB")) {
						std::stringstream ss;
						ss << getSize(kv.back());
						Sets->Put(kv.front(), ss.str());
					}
					else {
						Sets->Put(kv.front(), kv.back());
					}
					
				}
				else {
					callback->OnWarring(0, "������Ϣ�����쳣��ֵ����δ����", line);
				}
			}
			/*
			else if (StartWith(line, "VMFile=")) {
				std::string info = line.substr(7);
				void * chardata = malloc(sizeof(char)*info.size()+1);
				memcpy(chardata, info.c_str(), info.size() + 1);
				//Sets.VMPath = (char*)chardata;
			}
			else if (StartWith(line, "VMSize=")) {
				Sets.VMSize = (DWORD)getSize(line.substr(7));
			}
			else if (StartWith(line, "BlockSize=")) {
				Sets.BatchBlockSize = (DWORD)getSize(line.substr(10));
			}
			else if (StartWith(line, "SectionSize=")) {
				Sets.SectionSize = (DWORD)getSize(line.substr(12));
			}
			else if (StartWith(line, "EnableZip=")) {
				Sets.Compressable = getBool(line.substr(10));
				//std::cout << " Enable Compress :" << std::boolalpha << EnableCompress << std::endl;
			}
			else if (StartWith(line, "ZipIgnore=")) {
				std::string info = line.substr(10);
				void * chardata = malloc(sizeof(char)*info.size() + 1);
				memcpy(chardata, info.c_str(), info.size() + 1);
				//Sets.NOZipTarget = (char*)chardata;
			}
			else if (StartWith(line,"HugeFileSize=")) {
				Sets.HugeFileSize = getSize(line.substr(13));
			}*/
		}
	}

	static void SaveSetting(Setter * Sets, EventCallback * callback) {
		FileWriter fw;
		if (!fw.Open((char*)".\\setting.ini", 34 | std::ios_base::trunc)) {
			callback->OnError(GetLastError(), "FileWriter ' .\\setting.ini' ", "");
			//std::cout << " Load Setting File Error " << std::endl;
		}
		auto ptr = Sets->GetSrc();
		std::stringstream ss;
		for (auto e = ptr->begin(); e != ptr->end(); e++) {
			ss << e->first << "=" << e->second;
			fw.WriteLine(&ss.str());
			ss.str("");
			ss.clear();
		}

		//std::stringstream outstr;
		/*
		//outstr << "#�����ļ�·�� \n VMFile=" << Sets.VMPath;
		fw.WriteLine(&outstr.str());
		outstr.str("");
		outstr.clear();
		outstr << "#�����ļ���ʼ��С \n VMSize=" << Sets.VMSize;
		fw.WriteLine(&outstr.str());
		outstr.str("");
		outstr.clear();
		outstr << "#С�ļ���۳����ݿ����һ�ζ�д�Ĵ�С�����鲻����Ӳ����߶�д�ٶȵ�70%" <<
			"\n BlockSize=" << Sets.BatchBlockSize;
		fw.WriteLine(&outstr.str());
		outstr.str("");
		outstr.clear();
		outstr << "#���ļ����ζ�д�Ĵ�С������С��VMSize " <<
			"\n SectionSize=" << Sets.SectionSize;
		fw.WriteLine(&outstr.str());
		outstr.str("");
		outstr.clear();
		outstr << "#�Ƿ���������ѹ�� EnableZip=" << Sets.Compressable;
		fw.WriteLine(&outstr.str());
		outstr.str("");
		outstr.clear();
		//outstr << "#ͼƬ����Ƶ����Ƶ�ļ����Ǹ߶�ѹ���ĸ�ʽ�����������ʱ�ٴν���ѹ����Ч�治��" <<"\n ZipIgnore=" << Sets.NOZipTarget;
		fw.WriteLine(&outstr.str());
		outstr.str("");
		outstr.clear();
		outstr << "#�жϴ��ļ��ߴ����ʹ�С" <<
			"\n HugeFileSize=" << Sets.HugeFileSize;
		fw.WriteLine(&outstr.str());*/
	}


	static std::string ShowSetting(Setter Sets) {
		auto ptr = Sets.GetSrc();
		std::stringstream ss;
		for (auto e = ptr->begin(); e != ptr->end(); e++) {
			ss << e->first << "" << e->second <<"\n";
		}
		return ss.str();
	}
