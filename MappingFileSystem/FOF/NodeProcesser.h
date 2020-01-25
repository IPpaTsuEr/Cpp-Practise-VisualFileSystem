#pragma once
#include"Base.h"

#include<io.h>
#include <direct.h>
#include "Setter.h"


//////////////////////////////////判断属性////////////////////////////////////////
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

/////////////////////////////////////节点查找/////////////////////////////////////

		//获取节点下所有节点（文件对象节点 包括 batch、sequence方式写入的文件节点）
	static void getNodesUnderNode(PathNode * node, std::list<PathNode *> * list) {
		if (isHugeFile(node) || isBatchFile(node) || isNormalFile(node)) {
			list->push_back(node);
			//std::cout << "在节点 \t" << node->pat->info.name << "\t 取得 " << node->info.name << std::endl;
		}
		if (isFloder(node)) {
			if (node->sub != NULL && node->sub->size() > 0) {
				for (auto e = node->sub->begin(); e != node->sub->end(); e++) {
					//std::cout << "进入节点 \t" << e->info.name << std::endl;
					getNodesUnderNode(&(*e), list);
				}
			}
		}
	}
	//通过节点名称找到节点(仅限节点的初级子节点)
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
	//指定节点下是否存在指定文件目标
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
	//传入文件名返回其扩展名，若无扩展名返回Unknown
	static std::string getFileType(std::string fileName) {
		size_t DotIndex = fileName.find_last_of(".");
		if (DotIndex != fileName.npos) {
			return fileName.substr(DotIndex);
		}
		return "Unknown";
	}

	//获取本节点在父节点中的序号
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

	//获取外部文件路径下所有文件
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
			//优先处理文件防止递归过深
			if (!(data.attrib & _A_SUBDIR)) {//是文件
				std::string fn = data.name;
				size_t ind = fn.find_last_of(".");

				if (filter.empty() || filter.compare("*") == 0) {//过滤器为任意文件
					goto SAVEPOINT;
				}
				else {
					if (ind != fn.npos) {

						if (filter.find(fn.substr(ind)) != filter.npos) {//通过筛选器筛选
						//使用绝对路径
						SAVEPOINT:
							std::string savepath = outPath;
							if (EndWith(savepath, "\\"))savepath.append(data.name);
							else savepath.append("\\").append(data.name);

							result->push_back(savepath);
						}
					}
				}
			}
			else {//是文件夹 
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


/////////////////////////////////////路径处理/////////////////////////////////////
	//从文件路径中提取路径(若以\结尾，则认为路径就是文件夹)
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

	// 形如 C:/a/p.ocx 会被判定为文件路径，C:/a/p.ocx/ 则判定为文件夹路径 会创建带后缀名的文件夹
	static int MakeOutDirs(std::string AbsolutePath, std::string & original) {
		/*
		倒着压栈
		如 C:/a/b/c
		在函数递归栈中顺序为
		【栈底】C:/a/b/c
				C:/a/b
				C:/a
		【栈顶】C:
		然后 依次出栈，检测是否存在，不存在则创建
		*/
		size_t location = AbsolutePath.find_last_of("\\");

		if (location != AbsolutePath.npos) {
			//还有子路径，递归
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
		//是否存在
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

		//while (IsFileNodeExist(name, parentNode)) {//重名文件处理

		//	name = fn->name;//重置到原文件名
		//	index = name.find_last_of(".");//是否有后缀名
		//	if (index != name.npos && fn->type & FILE_TYPE) {//在后缀名前插入序号
		//		s_name << name.substr(0, index) << "_" << sequence << name.substr(index);
		//	}
		//	else {//对文件夹或无后缀名的文件直接插入序号 
		//		s_name << name << "_" << sequence;

		//	}
		//	name = s_name.str();//用于下次查找
		//	s_name.clear();
		//	s_name.str("");
		//	sequence++;//下一次插入的序号
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

		while (IsFileNodeExist(name, parentNode)) {//重名文件处理

			name = oname;//重置到原文件名
			index = name.find_last_of(".");//是否有后缀名
			if (index != name.npos && (fn->type & FILE_TYPE)) {//在后缀名前插入序号
				s_name << name.substr(0, index) << "_" << sequence << name.substr(index);
			}
			else {//对文件夹或无后缀名的文件直接插入序号 
				s_name << name << "_" << sequence;

			}
			name = s_name.str();//用于下次查找
			s_name.clear();
			s_name.str("");
			sequence++;//下一次插入的序号

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
	
/////////////////////////////路径/////////////////////////////////////////////
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
		//同磁盘下，可通过相对定位
			//真包含关系
			if (StartWith(FullPath, BasePath)) {
				FullPath.replace(0, BasePath.size(), replacer);
				return FullPath;
			}
			//部分包含关系
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
		//异磁盘下直接通过绝对定位
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

	
////////////////////////////////配置文件//////////////////////////////////////////

	static void LoadSetting(EventCallback * callback, Setter * Sets) {
		FileReader fr;
		if (!fr.Open((char*)".\\setting.ini")) {
			callback->OnError(GetLastError()," FileReader '.\\setting.ini' ",  "");
		}
		std::string line;
		while (fr.ReadLine(&line)) {
			if (StartWith(line, "#")) {
				//注释标记 不处理

			}
			else {
				line = ReplaceAll(line, " ");
				line = ReplaceAll(line, ";");
				std::list<std::string> kv = Split(line, "=");
				if (kv.size() > 2) {
					callback->OnError(0, "配置信息解析异常，同一属性设置了多个值", line);
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
					callback->OnWarring(0, "配置信息解析异常，值可能未设置", line);
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
		//outstr << "#虚拟文件路径 \n VMFile=" << Sets.VMPath;
		fw.WriteLine(&outstr.str());
		outstr.str("");
		outstr.clear();
		outstr << "#虚拟文件初始大小 \n VMSize=" << Sets.VMSize;
		fw.WriteLine(&outstr.str());
		outstr.str("");
		outstr.clear();
		outstr << "#小文件汇聚成数据块进行一次读写的大小，建议不超过硬盘最高读写速度的70%" <<
			"\n BlockSize=" << Sets.BatchBlockSize;
		fw.WriteLine(&outstr.str());
		outstr.str("");
		outstr.clear();
		outstr << "#大文件单次读写的大小，必须小于VMSize " <<
			"\n SectionSize=" << Sets.SectionSize;
		fw.WriteLine(&outstr.str());
		outstr.str("");
		outstr.clear();
		outstr << "#是否启用数据压缩 EnableZip=" << Sets.Compressable;
		fw.WriteLine(&outstr.str());
		outstr.str("");
		outstr.clear();
		//outstr << "#图片、音频、视频文件都是高度压缩的格式，不建议存入时再次进行压缩，效益不好" <<"\n ZipIgnore=" << Sets.NOZipTarget;
		fw.WriteLine(&outstr.str());
		outstr.str("");
		outstr.clear();
		outstr << "#判断大文件尺寸的最低大小" <<
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
