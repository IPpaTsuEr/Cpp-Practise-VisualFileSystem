#pragma once
#include"MapManager.h"
#include "NodeProcesser.h"
#include<list>
#include<map>
#include<io.h>
#include <direct.h>

class HDDProcesser {
	Setting * Sets;
	//LONGLONG FreeSize = 126 MB;
	MapManager * mm = NULL;
	EventCallback * callback;
	std::list<SpaceNode> BitMap;
	SystemHead SHead;
	std::string NOZipTarget = ".jpeg|.jpg|.png|.bmp|.tga|.tif|.gif|.mkv|.mp4|.wma|.avi|.rm|.rmvb|.flv|.mpg|.mov|.mp3|.ogg|.cue|.zip|.rar|.7z|.tag|.pdf";
	Setter * UserSets;

	//��·�������ڵ㣬���������սڵ������
	//path��\��β������Ϊ����Ϊ�ļ��нڵ㣬����Ϊ�ļ��ڵ�����������������ڵ�
	PathNode * _MakeDirs(std::string path, PathNode * node) {
		if (path.empty()) {
			callback->OnWarring(0,"_MakeDirs",getAbsoulutePath(node));
			return node;
		}

		size_t index = path.find_first_of("\\");
		std::string sub;

		if (index != path.npos) {
			sub = path.substr(0, index);
		}
		else {
			sub = path;
			index = 0;
			path = ".";
		}

		PathNode * pn = FindNodesInFloder(node, sub);
		if (pn == NULL) {
			PathNode * ps = CreateNewFloder(node,sub);
			ps->info.type |= INDEX_SUB_SIZECHANGED;
			pn = ps;

			if (path == ".") {
				callback->OnWarring(0, "_MakeDirs", getAbsoulutePath(pn));
				return pn;
			}
		}
		return _MakeDirs(path.substr(index + 1), pn);
	}

	PathNode * _getNodeByPath(std::string  path, PathNode * node) {

		if (path.empty())return node;

		size_t index = path.find_first_of("\\");
		std::string sub;

		if (index != path.npos) {//�����ӽڵ������
			sub = path.substr(0, index);
			PathNode * ps = FindNodesInFloder(node, sub);
			if (ps == NULL) return NULL;
			else return _getNodeByPath(path.substr(index + 1), ps);
		}
		else {
			return FindNodesInFloder(node, path);
		}

	}

	void _FindNode(std::list<PathNode *> * result, std::string targetName, PathNode* parent, bool likely) {
		if (isFloder(parent) || isFile(parent)) {
			if (likely) {
				std::string t;
				if (isLongName(parent))t = *GetLongName(parent);
				else t = parent->info.name;
				if (t.find(targetName) != t.npos)result->push_back(parent);
			}
			else {
				if (targetName.compare(parent->info.name) == 0)result->push_back(parent);
			}
			if (isFloder(parent) && parent->sub != NULL && parent->sub->size() > 0) {
				for (auto e = parent->sub->begin(); e != parent->sub->end(); e++) {
					_FindNode(result, targetName, &(*e), likely);
				}
			}
		}
	}


public:
	
	HDDProcesser(EventCallback * msg) {
		callback = msg;
		mm = new MapManager(callback);
		UserSets = new Setter();
		LoadSetting(callback,UserSets);
		//std::cout << ShowSetting(sets) << std::endl;;
		bool result = false;
		std::string NZ = UserSets->Convert<std::string>("UncompressableType", &result);
		std::list<std::string> list = Split(NZ, "|");
		for (auto e = list.begin(); e != list.end(); e++) {
			if (StartWith(*e, ".")) {
				if (NOZipTarget.find(*e)==(*e).npos)
						NOZipTarget.append(*e).append("|");
			}
		}
		callback->OnWarring( 0," DriverProcesser��ʼ�����","");
	}
	
	~HDDProcesser() {
		if (mm != NULL)delete mm;
		if (UserSets != NULL)delete UserSets;
		callback->OnWarring(0," DriverProcesser�˳�", "");
		callback = NULL;
	}

	void MergeNodes(PathNode* source,PathNode* destnation,bool rename) {
		if (isRoot(source) || isPartition(source))return;
		if (!isFloder(destnation))return;
		
		if (isFloder(source)) {
			auto t = FindNodesInFloder(destnation, source->info.name);
			if (t == NULL) {
				destnation->addChild(source);
			}
			else {
				for (auto i = source->sub->begin(); i != source->sub->end();) {
					MergeNodes(&(*i), t, rename);
					i = source->sub->erase(i);
				}
			}
		}
		else {
			if(rename)AutoRename(source, destnation);
			destnation->addChild(source);
		}
		source->pat->removeChild(source);
		destnation->addChild(source);
	}
/////////////////////////////////////�ڵ����/////////////////////////////////////

	//static bool ListCompare(PathNode * a, PathNode * b) {
	//	if (a->info.name > b->info.name)return true;
	//	return false;
	//}
	//ȫ�̲��Ҿ���ָ�����ƵĽڵ�(�ݹ�)
	std::list<PathNode *> GlobalSearch(std::string targetName, bool likely = false) {
		std::list<PathNode *> result;
		//result.sort(ListCompare);
		for (auto e = SHead.Root.sub->begin(); e != SHead.Root.sub->end(); e++) {
			auto t = LocalSearch(targetName, &(*e), likely);
			//t.sort(ListCompare);
			for (auto i =t.begin(); i != t.end(); i++)
			{
				result.push_back(*i);
			}
		}
		return result;
	}

	//��ָ���ڵ��²��Ҿ���ָ�����ƵĽڵ�(�ݹ��ӽڵ�)
	std::list<PathNode *> LocalSearch(std::string targetName, PathNode* parent, bool likely = false) {
		std::list<PathNode *> result;
		_FindNode(&result, targetName, parent, likely);
		return result;
	}

///////////////////////////////·���任///////////////////////////////////////////
	//������·�������ڵ㣬���������սڵ������
	//��Ϊ�����ϵĽڵ�ʼ��Ϊ�ļ��нڵ�
	PathNode* MakeDirs(std::string AbsolutePath) {
		PathNode* partition = getPartition(AbsolutePath.c_str()[0]);
		std::string sub = AbsolutePath.substr(3);//���̷���·��
		return _MakeDirs(sub, partition);
	}

	//�����·�������ڵ㣬���������սڵ������
	//��Ϊ·���ϵĽڵ�ʼ��Ϊ�ļ��нڵ�
	PathNode * MakeDirs(PathNode * node, std::string RelativePath) {
		if (StartWith(RelativePath, "\\"))RelativePath = RelativePath.substr(1);
		return _MakeDirs(RelativePath, node);
	}

	//��������·��Ϊ�ļ��������ȡ�����ļ�����
	PathNode * getNodeByPath(std::string AbsolutePath) {
		PathNode* partition = getPartition(AbsolutePath);
		std::string subpath = AbsolutePath.substr(3);//���̷���·��
		return _getNodeByPath(subpath, partition);
	}

	//��������·��Ϊ�ļ��������ȡ�����ļ�����
	PathNode * getNodeByPath(PathNode * node, std::string RelativePath) {
		//���·�������̷���ֱ�Ӿ��Զ�λ
		std::string abBase = getAbsoulutePath(node);
		if (StartWith(RelativePath, ":\\", 1) && abBase.at(0) != RelativePath.at(0))
			return getNodeByPath(RelativePath);


		if (StartWith(RelativePath, "\\"))RelativePath = RelativePath.substr(1);
		std::list<std::string> list = Split(RelativePath, "\\");
		auto spt = list.begin();
		PathNode * target = node;
		while (spt != list.end())
		{
			if (spt->compare(".")==0) {
				target = target->pat;
			}
			else {
				target = FindNodesInFloder(target,*spt);
			}
			if (target == NULL) return NULL;
			spt++;
		}
		return target;
		//return _getNodeByPath(RelativePath, node);
	}

////////////////////////////////////Partition�ڵ����////////////////////////////////////////////////////

	PathNode * getPartition(unsigned int index) {
		if (index >= SHead.Root.sub->size()) {
			return NULL;
		}
		return at<PathNode>(index, SHead.Root.sub);
	}

	PathNode * getPartition(char index) {
		if (index > 'Z')index -= 32;
		for (auto e = SHead.Root.sub->begin(); e != SHead.Root.sub->end(); e++)
		{
			if (index == e->info.a_time)return &(*e);
		}
		return NULL;
	}

	PathNode * getPartition(std::string Disk) {
		switch (Disk.size()) {// C:/
		case 0://��·��
		case 1://ֻ���̷�
		case 2:return NULL;//·��������
		default:return getPartition(Disk.c_str()[0]);//�����̷�·��
		}
	}

	PathNode * getPartitionRoot() {
		return &SHead.Root;
	}

///////////////////////////////////��������////////////////////////////////////////////////////	
	
	bool CreatePartition(std::string name="�½���",unsigned char index ='C') {
		PathNode P;
		P.info.setName(name);
		if (SHead.Root.sub == NULL)SHead.Root.sub = new std::list<PathNode>();
		for(auto e = SHead.Root.sub->begin();e!= SHead.Root.sub->end();e++)
		{
			if (index  == e->info.a_time)return false;
		}
		P.pat =&SHead.Root;
		P.sub = new std::list<PathNode>();
		P.info.a_time = index ;//�̷�
		P.info.c_time = P.info.m_time = time(0);
		P.info.type = FLODER_TYPE | PARTITION_TYPE;
		SHead.Root.addChild(&P);
		return true;
	}

	size_t getPartitionSize() {
		return SHead.Root.sub->size();
	}

	size_t getFileSize(std::list<std::string>list) {
		size_t size = 0;
		for (auto i = list.begin(); i != list.end(); i++) {
			size = getFileInfo(*i).data.count;
		}
		return size;
	}
	
	size_t getFileSize(std::string path){
		return getFileInfo(path).data.count;
	}

////////////////////////////////�ռ����//////////////////////////////////////////////////
	
	void ReleaseSpace(SpaceNode node) {
		if (node.count > 0 && node.location > 0) {
			BitMap.push_front(node);
			SHead.FreeSize += node.count;

			std::stringstream sss;
			sss << "  F: " << node.location << " T: " << node.count + node.location << " S: " << ConvertSpaceToRead(node.count);
			callback->OnWarring(0,"�ͷſռ�",sss.str());
		}
	}

	void ReleaseSpace(PathNode * node) {
		if (node->sub != NULL && node->sub->size() > 0) {
			std::list<PathNode>::iterator it = node->sub->begin();
			while (it != node->sub->end()) {
				ReleaseSpace(&(*it));
				it++;
			}
			node->sub->clear();
			std::list<PathNode> tmp;
			node->sub->swap(tmp);
		}
		BitMap.push_front(node->info.data);

		std::stringstream sss;
		sss << "  F: "<<node->info.data.location<<" T: "<< node->info.data.count + node->info.data.location << " S: " <<ConvertSpaceToRead(node->info.data.count);
		callback->OnWarring( 0,"�ͷſռ�",sss.str());
		//�����ڽڵ�ĸ��ڵ�ɾ���Ա��ڵ�Ĺ���
		//ʹ��
		//size_t index=getIndexInParent(&node);
		//node.pat->erase(node.pat->begin() + index);
		//std::cout << " Current Space Node :" << BitMap.size() << std::endl;
	}

	void MergaSpace() {
		std::list<SpaceNode>::iterator oit = BitMap.begin(),iit;
		while (oit != BitMap.end()) {
			iit = oit;
			iit++;
			while (iit != BitMap.end()) {
				if (oit->count != 0 && iit->count != 0) {
					if (oit->location + oit->count  == iit->location) {
						oit->count += iit->count;
						iit->count = 0;
					}
					if (iit->location+ iit->count == oit->location) {
						iit->count += oit->count;
						oit->count = 0;
					}
				}
			
				iit++;
			}
			oit++;
		}
		oit = BitMap.begin();
		while (oit != BitMap.end()) {
			if (oit->count == 0)oit = BitMap.erase(oit);
			else oit++;
		}
	}

	SpaceNode HoldSpace(size_t length) {
		//std::cout << " Current Space Node :" << BitMap.size() << "\t" << std::endl;
		//for (auto e = BitMap.begin(); e != BitMap.end(); e++) {
		//	std::cout << "\t F: " << e->location << " L: " << e->count << " T: " << e->count + e->location << std::endl;
		//}
		//std::cout << "\t Return Node F: " << node.location << " L: " << node.count << " T: " << node.count + node.location << std::endl;


		std::list<SpaceNode>::iterator	dit = BitMap.end(), it = BitMap.begin(), bt = BitMap.begin();
		SpaceNode Nearest,Bigest;
		LONGLONG diff  = 0xfffffffffffffffLL;
		while (it != BitMap.end()) {
			if (it->count >= Bigest.count) {
				Bigest = *it;
				bt = it;
			}
			if ((size_t)it->count > length) {
				if ((size_t)diff > it->count - length) {
					diff = it->count - length;
					Nearest = *it;
					dit = it;
				}
			}
			if (it->count == length) {
				Nearest = *it;
				it = BitMap.erase(it);
				SHead.FreeSize -= Nearest.count;
				return Nearest;
			}
			it++;
		}
		SpaceNode node;

		if (dit == BitMap.end()) {//û���ҵ����ʵĿռ�
			//���зֿ�洢
			std::stringstream ss;
			ss << "��Ҫ�����ռ��С��" << length << " ��ǰ���������ռ�����С��" << Bigest.count;
			callback->OnWarring(0, " �����ռ䲻�� ", ss.str());
			SpaceNode dsp;
			dsp.blockindex = 0;
			dsp.location = mm->getFormatSize();
			dsp.count = length;
			if (mm->UpSize(length)) {
				SHead.Sets.VMSize = dsp.location + length;
				return dsp;
			}
			else {
				ss.clear();
				ss.str("");
				ss << " �����С��" << ConvertSpaceToRead(length);
				callback->OnError(0, "���������ռ��Сʧ��", ss.str());
				throw  ERROR_HOLD_SPACE;
			}
		}
		else {
			BitMap.erase(dit);
		}
			
		node.location = Nearest.location;
		node.count = length;
		node.blockindex = 0;
		Nearest.location = Nearest.location + length;
		Nearest.count = Nearest.count - length;
		BitMap.push_front(Nearest);
		SHead.FreeSize -= node.count;


		std::stringstream sss;
		sss << "F: " << node.location << " T: " << node.count + node.location << " S: " << ConvertSpaceToRead(node.count);
		callback->OnWarring( 0," ����ռ� ",sss.str());
		return node;
	}


	void ReleaseNormalFile(PathNode *node) {
		SpaceNode sp = node->info.data;
		if (node->info.zipedSize > 0) {
			sp.count = node->info.zipedSize;
		}
		ReleaseSpace(sp);
		ReleaseLongName(node);
		node->pat->info.type |= INDEX_SUB_SIZECHANGED;
		node->pat->sub->remove(*node);

	}
	
	void ReleaseHugeFile(PathNode *node) {
		SpaceNode sp = node->info.data;
		Shift sf = mm->GetView<SpaceNode>(sp);
		SpaceNode* ptr = (SpaceNode*)sf.getEntry();
		for (LONGLONG e = 0; e < sp.count ; e++) {
			SpaceNode tp;
			tp.location = ptr[e].location;
			if (ptr[e] .blockindex != ptr[e].count) {
				tp.count = ptr[e].blockindex;
			}
			else {
				tp.count = ptr[e].count;
			}
			ReleaseSpace(sp);
		}
		ReleaseLongName(node);
		node->pat->info.type |= INDEX_SUB_SIZECHANGED;
		node->pat->sub->remove(*node);
		
	}
	
	void ReleaseBatchFile(PathNode* node) {
		PathNode * bnode = GetTargetBatchNode( FindBatchNode(node), node->info.data.blockindex);
		SpaceNode sp;
		//׼�����ݻ���
		size_t OriginalSize = bnode->info.data.count;
		sp.location = bnode->info.data.location;
		void* Buffer =NULL;
		//���ļ��ж�ȡ���ݿ飬�����ѹ���ľͽ�ѹ
		
		if (bnode->info.zipedSize > 0 ) {
			sp.count = bnode->info.zipedSize;
			Shift sf = mm->GetView(sp);

			if (UnZipData(sf.getEntry(), sp.count, &Buffer, &OriginalSize)) {
				mm->UnView(sf);
			}
			else {
				callback->OnError(4879,"��ɾ��Batch���ݿ��е��ļ�����ʱ���ִ������ݿ鱻ѹ������ѹʧ��",bnode->info.name );
				return;
			}
		}
		else {
			sp.count = OriginalSize;
			Shift sf = mm->GetView(sp);
			Buffer = malloc(OriginalSize);
			memcpy(Buffer, sf.getEntry(), OriginalSize);
			mm->UnView(sf);
		}
		
		//-----------------------------��С���ݿ��С������д���ļ�
		//ӳ��ƫ���б���ֱ�����ڴ����޸ģ���ӳ��ʱ���Զ�Ӧ�õ��ļ�
		
		LONGLONG * ptr = (LONGLONG* )bnode->info.name;
		SpaceNode lsp;
		lsp.location = ptr[SPL_START];
		lsp.count = ptr[SPL_SIZE];
		Shift lsf = mm->GetView(lsp);
		SpaceNode * SPptr = (SpaceNode*)lsf.getEntry();
		
		//�����µ����ݶ�
		size_t OutDataSize = OriginalSize - node->info.data.count;
		void* OutData = malloc(OutDataSize);
		size_t IndexSize = lsp.count / sizeof(SpaceNode);
		size_t Offset = 0;
		//�޸�ƫ���б��еļ�¼ ���ƶ�����
		for (size_t Index = 0; Index < IndexSize ; Index++) {
			if (Index != node->info.data.location) {
				memcpy((BYTE*)OutData+Offset,(BYTE*)Buffer+SPptr[Index].location ,SPptr[Index].count);
				SPptr[Index].location = Offset;
				Offset += SPptr[Index].count;
				//if(Index> node->info.data.location)
				//�޸Ķ�Ӧ�ڵ���ƫ�ƣ�������ƫ�Ʊ�Ĵ�С[��Ϊ�ļ��µĶ�ӦIndex����֮ǰ�����]
					
				std::cout << " Index :" << Index << " Location:" << SPptr[Index].location << " Count:" << SPptr[Index].count << " BIndex:" << bnode->info.data.blockindex << std::endl;

			}
		}
		SPptr[node->info.data.location].location = 0;
		SPptr[node->info.data.location].count = 0;
		SPptr[node->info.data.location].blockindex = -10;

		free(Buffer);
		mm->UnView(lsf);
		ReleaseSpace(sp);

		//�������ݿ�д���ļ�
		void * OutZipData = NULL;
		size_t OutZipSize = 0;
		SpaceNode nsp;
		Shift nsf;
		if (Sets->Compressable && ZipData(OutData, OutDataSize,&OutZipData,&OutZipSize)) {
			nsp = HoldSpace(OutZipSize);
			nsf = mm->GetView(nsp);
			memcpy(nsf.getEntry(), OutZipData, OutZipSize);
			free(OutZipData);
			bnode->info.zipedSize = OutZipSize;
		}
		else {
			nsp = HoldSpace(OutDataSize);
			nsf = mm->GetView(nsp);
			memcpy(nsf.getEntry(), OutData, nsp.count);
			bnode->info.zipedSize = 0;
		}
		bnode->info.data.location = nsp.location;
		bnode->info.data.count = OutDataSize;
		mm->UnView(nsf);
		free(OutData);

		ReleaseLongName(node);
		node->pat->info.type |= INDEX_SUB_SIZECHANGED;
		node->pat->sub->remove(*node);
		
	}

	void ReleaseIndexSpace(PathNode * node) {
		if (isFloder(node)) {
			SpaceNode sp = node->info.data;
			sp.count = sp.count * sizeof(FileInfo);

			node->pat->sub->remove(*node);
		}
	}

////////////////////////////////New VMF//////////////////////////////////////////
	LONGLONG getFreeSize() { return SHead.FreeSize; }

	bool OpenVMFile(std::string VMFilePath, bool withformat,Setting* sets) {
		if (mm->Open(VMFilePath)) {
			callback->OnWarring(0, "���ļ��ɹ�",VMFilePath);
			if (withformat) {
				FormatVMFile(sets);
			}
			else {
				LoadTreeHead();
				if (SHead.Sets.Mark != FORMAT_SYMBOL)return false;
				callback->OnWarring(0, "---��ʼIndex����---", "");
				LoadIndexTree(&SHead.Root);
				callback->OnWarring(0, "---Index�������---", "");
			}
			Sets = &SHead.Sets;
			
			return true;
		}
		return false;
	}

	void CloseVMFile() {
		ReleaseSpace(SHead.EmptySpaces);
		//��ȡ���Ŀռ�һ���ǵ��ڻ���ڸ�BitMap.size()��
		SpaceNode sp = SaveSpaceList(&BitMap);
		SHead.EmptySpaces = sp;
		callback->OnWarring(0, "---��ʼIndexд��---", "");
		SaveIndexTree(&SHead.Root);
		callback->OnWarring(0, "---Indexд�����---", "");
		mm->Close();
	}

	bool CreateVMFile(std::string path,Setting * sets,size_t * Completion) {
		MapManager* mmr = MapManager::Create(callback, path, sets->VMSize, Completion);
		std::cout << "Static Call End";
		if(mmr!=NULL) {
				SystemHead sh;
				Setter st;
				st.ConvertSetting(*sets);
				SetSystemHeadFormSetter(&sh,&st,NULL);
				sh.FreeSize = sets->VMSize - sizeof(SystemHead);
				sh.EmptySpaces.location = 0;//����ʼ�����

				SpaceNode sp;
				sp.location = 0;
				sp.count = 1;
				Shift sf = mmr->GetView<SystemHead>(sp);
				memcpy(sf.getEntry(),&sh,sizeof(SystemHead));
				mmr->UnView(sf);


				return true;
		}
		return false;
	}

	void FormatVMFile(Setting *sets = NULL) {
		Setter* setter = new Setter();
		
		if (sets == NULL) {
			LoadSetting(callback, setter);
		}
		else {
			setter->ConvertSetting(*sets);
		}
		SetSystemHeadFormSetter(&SHead, setter, &NOZipTarget);
		SHead.Sets.VMSize = mm->getFormatSize();
		SHead.Sets.Version = FORMAT_VERSION;

		std::stringstream hddcode;
		hddcode << time(0);
		SHead.Root.info.setName(hddcode.str());
		SHead.Root.info.m_time = SHead.Root.info.c_time = time(0);
		SHead.Root.info.type = FLODER_TYPE | ROOT_TYPE;
		SHead.Root.info.data = { 0,0,0 };
		SHead.Root.sub = new std::list<PathNode>();
		SHead.Root.pat = NULL;
		
		SHead.EmptySpaces = {0,0,0} ;
		SpaceNode sp;
		sp.location = sizeof(SystemHead);
		sp.count = mm->getFormatSize()- sp.location;
		BitMap.push_front(sp);
		SHead.FreeSize = sp.count;
		SaveTreeHead();

		std::stringstream sss;
		sss << " ���ÿռ�: " << ConvertSpaceToRead(SHead.Sets.VMSize - sizeof(SystemHead));
		callback->OnWarring(0,"��ʽ�����",sss.str() );
	}
	
	void SaveTreeHead() {
		SpaceNode ssp = SaveSpaceList(&BitMap);
		SHead.EmptySpaces = ssp;

		if (SHead.Root.sub != NULL && SHead.Root.sub->size()>0) {
			SpaceNode sp = HoldSpace(SHead.Root.sub->size() * sizeof(FileInfo));
			Shift sf = mm->GetView(sp);
			size_t size = 0;
			for (auto e :* SHead.Root.sub) {
				memcpy(sf.getEntry() + size, &e.info, sizeof(FileInfo));
				size += sizeof(FileInfo);
			}
			SHead.Root.info.data.location = sp.location;
			SHead.Root.info.data.count = SHead.Root.sub->size();
		}

		SpaceNode need;
		need.location = 0;
		need.count = 1;
		Shift ptr = mm->GetView<SystemHead>(need);
		memcpy(ptr.getEntry(), &SHead, sizeof(SystemHead));
		mm->UnView(ptr);

		std::stringstream sss;
		sss << " д��ͷ�ڵ�: " << SHead.Root.info.name << " F: " << need.location << " T: " << need.location + sizeof(SystemHead);
		callback->OnWarring( 0,sss.str(),"");
	}

	void LoadTreeHead() {
		SpaceNode need;
		need.location = 0;
		need.count = 1;
		Shift ptr = mm->GetView<SystemHead>(need);
		memcpy(&SHead, ptr.getEntry(), sizeof(SystemHead));
		mm->UnView(ptr);

		if (SHead.EmptySpaces.location == 0) {
			SpaceNode sp;
			sp.location = sizeof(SystemHead);
			sp.count = SHead.Sets.VMSize - sp.location;
			BitMap.push_back(sp);
		}
		else {

			LoadSpaceList(SHead.EmptySpaces, &BitMap);
		}
		SHead.Root.sub = NULL;
		//LoadIndexTree(&SHead.Root);

		std::stringstream sss;
		sss << " ����ͷ�ڵ�: " << SHead.Root.info.name << " F: " << need.location << " T: " << need.location + sizeof(SystemHead);
		callback->OnWarring( 0,sss.str(),"");
	}
	
	void SetSystemHeadFormSetter(SystemHead * head, Setter * sets,std::string * Exstr) {

		bool result = false;
		Setting defaultValue;
		head->Sets.BatchBlockSize = sets->Convert<DWORD>("BatchBlockSize", &result);
		if (!result)head->Sets.BatchBlockSize = defaultValue.BatchBlockSize;
		head->Sets.HugeFileSize = sets->Convert<DWORD>("HugeFileSize", &result);
		if (!result)head->Sets.HugeFileSize = defaultValue.HugeFileSize;
		head->Sets.SectionSize = sets->Convert<DWORD>("SectionSize", &result);
		if (!result)head->Sets.SectionSize = defaultValue.SectionSize;
		head->Sets.Compressable = sets->Convert<bool>("Compressable", &result);
		if (!result)head->Sets.Compressable = defaultValue.Compressable;
		
		if(Exstr != NULL) {
			std::string NZ = UserSets->Convert<std::string>("UncompressableType", &result);
			std::list<std::string> list = Split(NZ, "|");
			for (auto e = list.begin(); e != list.end(); e++) {
				if (StartWith(*e, ".")) {
					if (NOZipTarget.find(*e) == (*e).npos)
						NOZipTarget.append(*e).append("|");
				}
			}
		}
	}

////////////////////////////////New Index//////////////////////////////////////////
	//������ڵ㣬�ݹ����нڵ㲢���浽�ļ�
	void SaveIndexTree(PathNode * Root) {
		if (StartWith(Root->info.name,"BATCH")) {
			std::cout << "Bilibili";
		}

		if (Root->info.type & INDEX_NAME_CHANGE) {
			SetLongName(Root,*GetLongName(Root));
		}
		//ɾ�����ļ����ĵ�ַ����
		UnloadLongName(Root);
		
		//�ݹ鵽����Ľڵ�
		if (isSubGetable(Root) ) {
			if (Root->sub != NULL && Root->sub->size() > 0) {
				for (auto e = Root->sub->begin(); e != Root->sub->end(); e++) {
					SaveIndexTree(&(*e));
				}
			}
		}
		
		if (Root->info.type & INDEX_SUB_SIZECHANGED && !isRoot(Root)) {
				ReleaseSpace(Root->info.data);

				size_t size = Root->sub->size() * sizeof(FileInfo);
				SpaceNode sp = HoldSpace(size);
				Shift sf = mm->GetView(sp);

				std::stringstream ss;
				ss << Root->info.name << " L " << sp.location << "  C " << sp.count << " S " << Root->sub->size();
				callback->OnWarring(0, " д��Index ", ss.str());

				size = 0;
				for (auto c = Root->sub->begin(); c != Root->sub->end(); c++) {
					memcpy(sf.getEntry() + size, &(c->info), sizeof(FileInfo));
					size += sizeof(FileInfo);
				}
				mm->UnView(sf);
				Root->info.data.location = sp.location;
				Root->info.data.count = Root->sub->size();
				Root->info.type ^= INDEX_SUB_SIZECHANGED;
				//��֤data�ֶ��ܱ�����
				Root->info.type |= INDEX_SELF_MODIFY;


			}
		else if (Root->info.type & INDEX_SUB_MODIFY && !isRoot(Root)) {
				Shift sf = mm->GetView<FileInfo>(Root->info.data);
				size_t Offset = 0;
				for (auto c = Root->sub->begin(); c != Root->sub->end(); c++) {
					memcpy(sf.getEntry() + Offset, &(c->info), sizeof(FileInfo));
					Offset += sizeof(FileInfo);
				}
				mm->UnView(sf);
				Root->info.type ^= INDEX_SUB_MODIFY;
				Root->info.type |= INDEX_SELF_MODIFY;

				std::stringstream ss;
				ss << Root->info.name << " L " << Root->info.data.location << "  C " << Root->info.data.count << " S " << Root->sub->size();
				callback->OnWarring(0, " ����Index ", ss.str());
			}
		if (Root->info.type & INDEX_SELF_MODIFY && !isRoot(Root)) {
				Root->pat->info.type |= INDEX_SUB_MODIFY;
				Root->info.type ^= INDEX_SELF_MODIFY;

			}
		if (isRoot(Root)) {SaveTreeHead();}

		/*else {
			std::cout << " NULL " << Root->info.name << std::endl;
			std::stringstream ss;
			ss << Root->info.type;
			callback->OnError(2133,"δ֪�Ľڵ���Ϣ������", ss.str());
		}*/

	}

	//��Index�����ļ����뵽ָ���Ľڵ���
	void LoadIndexTree(PathNode * Root) {

		if (isSubGetable(Root)) {

			SpaceNode sp = Root->info.data;
			Shift sf = mm->GetView<FileInfo>(sp);

			std::stringstream ss;
			ss << Root->info.name << " L " << sp.location << " C " << sp.count* sizeof(FileInfo) << " S " << sp.count;
			callback->OnWarring(0, " ����Index ", ss.str());

			for (LONGLONG i = 0; i < Root->info.data.count; i++) {
				PathNode ps;
				memcpy(&ps.info,sf.getEntry() + i * sizeof(FileInfo), sizeof(FileInfo));
				LoadLongName(&ps);//���볤�ļ������ڴ�
				Root->addChild(&ps);
			}
			mm->UnView(sf);

			if (Root-> sub != NULL && Root->sub->size() > 0) {
				for (auto e = Root->sub->begin(); e != Root->sub->end(); e++) {
					if(e->info.data.count!=0)LoadIndexTree(&(*e));
				}
			}
		}
		else {
			std::stringstream ss;
			ss << Root->info.name << "�ڵ��������ݶ�";
			callback->OnWarring(0,ss.str() , "�������ӽڵ�");
		}
	}

	void SetLongName(PathNode* node,std::string name){
		 SetLongName(&node->info, name);
	}
	
	void SetLongName(FileInfo * info, std::string name) {
		if (info->type & LONG_NAME)ReleaseLongName(info);
		//BYTE == CHAR =1 
		SpaceNode sp = HoldSpace(name.size() + 1);
		Shift sf = mm->GetView(sp);
		memcpy(sf.getEntry(), name.c_str(), name.size() + 1);
		mm->UnView(sf);
		LONGLONG * ptr = (LONGLONG *)info->name;
		ptr[LN_START] = sp.location;
		ptr[LN_SIZE] =  sp.count;
		ptr[LN_PTR] = NULL;
		info->type |= LONG_NAME;
		SetShortName(info, name);
	}
	
	void SetShortName(FileInfo * info,std::string name) {
		if ((name.size()+1) > NAME_SIZE) {
			size_t tSize = NAME_SIZE - (sizeof(LONGLONG) * 3);
			memcpy(info->name, name.c_str(), tSize - 1);
			info->name[tSize-1] = '~';
			info->name[tSize] = '\0';
		}
	}
	
	void SetShortName(PathNode* node, std::string name) {
		SetShortName(&node->info, name);
	}

	std::string * GetLongName(PathNode* node) {
		if (!isLongName(node))return NULL;
		LONGLONG * ptr = (LONGLONG *)node->info.name;
		return (std::string *)ptr[LN_PTR];
	}
	//�����ļ������뵽�ڴ棬������ָ��
	void LoadLongName(PathNode* node) {
		if (!isLongName(node))return;
		LONGLONG * ptr = (LONGLONG *)node->info.name;
		SpaceNode sp;
		sp.location = ptr[LN_START];
		sp.count = ptr[LN_SIZE];
		Shift sf = mm->GetView(sp);
		std::string * sptr = new std::string((char *)sf.getEntry(), ptr[LN_SIZE]-1);
		mm->UnView(sf);
		ptr[LN_PTR] = (LONGLONG)sptr;
	}
	//
	void UnloadLongName(PathNode* node) {
		if (!isLongName(node))return;
		LONGLONG * ptr = (LONGLONG *)node->info.name;
		std::string * sptr = (std::string *)ptr[LN_PTR];
		if(sptr !=NULL)delete sptr;
	}

	void ReleaseLongName(PathNode* node) {
		ReleaseLongName(&node->info);
	}

	void ReleaseLongName(FileInfo * info) {
		if (!(info->type & LONG_NAME))return;
		SpaceNode sp;
		LONGLONG * ptr = (LONGLONG *)info->name;
		if (ptr[LN_START] != NULL && ptr[LN_SIZE] != NULL) {
			sp.location = ptr[LN_START];
			sp.count = ptr[LN_SIZE];
			ReleaseSpace(sp);
			ptr[LN_START] = NULL;
			ptr[LN_SIZE] = NULL;
		}
		
	}

	void Rename(PathNode* node, std::string newname) {
		if (!node->info.setName(newname))SetLongName(node,newname);
	}

	//��ȡ�ļ���Ϣ
	FileInfo getFileInfo(std::string FilePath, ULONG type = INDEX_NORMAL) {

		struct _stati64 file;
		_stati64(FilePath.c_str(), &file);

		FileInfo fi;

		fi.data.count = file.st_size;
		fi.m_time = file.st_mtime;
		fi.a_time = file.st_atime;
		fi.c_time = file.st_ctime;
		//fi.type = file.st_mode;
		fi.type = type | FILE_TYPE;
		std::string name = FilePath.substr(FilePath.find_last_of("\\") + 1);
		if (!fi.setName(name)) {
			SetLongName(&fi,name);
		}
		//memcpy(fi.name, name.c_str(), name.length());
		return fi;
	}

	PathNode * CreateNewFloder(PathNode * parent,std::string name) {
		PathNode pn;
		pn.pat = parent;
		pn.info.c_time = pn.info.a_time = pn.info.m_time = time(0);
		if (!pn.info.setName(name))SetLongName(&pn, name);
		AutoRename(&pn, parent);
		pn.sub = new std::list<PathNode>();
		pn.info.type = FLODER_TYPE;
		parent->addChild(&pn);
		parent->info.type |= INDEX_SUB_SIZECHANGED;
		return &parent->sub->back();
	}

////////////////////////////////New Reader//////////////////////////////////////////
	//������д���ⲿ�ļ�
	void Reader(std::string OutSaveBase,std::list<PathNode*> * list,PathNode * BaseNode, bool OverWrite = false) {
		std::list<PathNode*> BatchList;
		std::list<PathNode*> NormalList;
		std::list<PathNode*> HugeList;
		for (auto e = list->begin(); e != list->end(); e++) {
			if (isBatchFile(*e))BatchList.push_back(*e);
			else if (isNormalFile(*e))NormalList.push_back(*e);
			else if (isHugeFile(*e))HugeList.push_back(*e);
			else { callback->OnError(2333,"δ֪�������ͣ��޷�д�����ݵ�ָ��λ��",(*e)->info.name); }
		}
		callback->OnWarring(0, "---��ʼ�ļ�д��---", "");
		HugeRead(OutSaveBase, &HugeList, BaseNode, OverWrite);
		NormalRead(OutSaveBase, &NormalList, BaseNode ,OverWrite);
		BatchRead(OutSaveBase, &BatchList, BaseNode, OverWrite);
		callback->OnWarring(0, "---�ļ�д�����---", "");
	}
	
	void BatchRead(std::string OutSaveBase, std::list<PathNode*> * BatchList, PathNode * BaseNode, bool OverWrite = false) {
		std::map<LONGLONG,std::list< PathNode*>> MapList;
		size_t TotalSize = GroupBatchNodes(BatchList, &MapList);
		//�жϴ��̿ռ��Ƿ��㹻�ļ�д��

		PathNode * RootBatch = FindBatchNode(BaseNode);

		for (auto e = MapList.begin(); e != MapList.end(); e++) {
			
			PathNode * bnode  = GetTargetBatchNode(RootBatch, e->first);
			//�������ݿ�
			SpaceNode sp;
			size_t OriginalSize = bnode->info.data.count;
			void * Buffer = NULL;// malloc(bnode->info.data.count);
			
			sp.location = bnode->info.data.location;
			if (bnode->info.zipedSize > 0) {
			//���ݱ�ѹ��
				sp.count = bnode->info.zipedSize;
				Shift sf = mm->GetView(sp);
				
				if (UnZipData(sf.getEntry(), sp.count, &Buffer, &OriginalSize)) {
					mm->UnView(sf);
				}
				else {
					callback->OnError(2334,"��ѹBatch���ݿ����", bnode->info.name);
					mm->UnView(sf);
					continue;
				}
			}
			else {
				sp.count = OriginalSize;
				Buffer = malloc(OriginalSize);
				Shift sf = mm->GetView(sp);
				memcpy(Buffer,sf.getEntry(),OriginalSize);
				mm->UnView(sf);
			}

			LONGLONG * sq = (LONGLONG *)bnode->info.name;
			SpaceNode slp;
			slp.location = sq[SPL_START];
			slp.count = sq[SPL_SIZE];
			std::list<SpaceNode> snl;
			LoadSpaceList(slp,&snl);
			SpaceNode * spp = NULL;

			for (auto i = e->second.begin(); i != e->second.end(); i++) {
				HANDLE fh = MakeOutTarget(OutSaveBase, BaseNode, *i, OverWrite);
				SpaceNode dsp = (*i)->info.data;
				
				for (auto ces : snl) {
					if (dsp.location == ces.blockindex) {
						spp = &ces;
						break;
					}
				}

				std::cout << " ��ȡƫ�� " << spp->location << "  ��С "<< dsp.count << " NT:"<< spp->location  + dsp.count<<std::endl;
				WriteFile(fh, (BYTE*)Buffer + spp->location, dsp.count, NULL, NULL);
				CloseHandle(fh);
				callback->OnWarring(0, " �ļ���ȡ��ϣ�", (*i)->info.name);
			}
			free(Buffer);
		}

	}

	void NormalRead(std::string OutSaveBase, std::list<PathNode*> * NormalList, PathNode * BaseNode, bool OverWrite = false) {
		for (auto e = NormalList->begin(); e != NormalList->end(); e++) {
			PathNode * file= *e;
			SpaceNode sp = file->info.data;
			Shift sf;

			HANDLE fh = MakeOutTarget(OutSaveBase, BaseNode, file,OverWrite);

			if (file->info.zipedSize > 0) {
				sp.count = file->info.zipedSize;
				size_t OriginalSize = file->info.data.count;
				void * Buffer = malloc(OriginalSize);
				sf = mm->GetView(sp);
				if (UnZipData(sf.getEntry(), sp.count, &Buffer, &OriginalSize)) {
					mm->UnView(sf);
					WriteFile(fh, Buffer, OriginalSize, NULL, NULL);
					free(Buffer);
				}
				else {
					mm->UnView(sf);
					free(Buffer);
					callback->OnError(2334,"��ѹBatch���ݿ����", file->info.name);
					continue;
				}
			}
			else {
				sf = mm->GetView(sp);
				WriteFile(fh, sf.getEntry(), sp.count, NULL, NULL);
				mm->UnView(sf);
			}

			CloseHandle(fh);
		}
	}

	void HugeRead(std::string OutSaveBase, std::list<PathNode*> * HugeList, PathNode * BaseNode, bool OverWrite = false) {
		for (auto e = HugeList->begin(); e != HugeList->end(); e++) {
			PathNode * file = *e;
			size_t spsize = file->info.data.count * sizeof(SpaceNode);
			SpaceNode * ptr =(SpaceNode*)malloc(spsize);
			SpaceNode sp = file->info.data;
			sp.count = spsize;
			Shift sf = mm->GetView(sp);
			memcpy(ptr, sf.getEntry(), spsize);
			mm->UnView(sf);

			HANDLE fh = MakeOutTarget(OutSaveBase,BaseNode,file,OverWrite);

			for (LONGLONG index = 0; index < file->info.data.count; index++) {
				size_t OriginalSize = ptr[index].blockindex;
				
				if (ptr[index].blockindex != ptr[index].count ) {
					//���ݶα�ѹ��	
					void * Buffer = malloc(OriginalSize);
					Shift sf = mm->GetView(ptr[index]);
					if (UnZipData(sf.getEntry(), ptr[index].count,&Buffer, &OriginalSize)) {
						mm->UnView(sf);
						WriteFile(fh, Buffer, OriginalSize, NULL, NULL);
						free(Buffer);
					}
					else {
						mm->UnView(sf);
						free(Buffer);
						callback->OnError(2334,"��ѹHuge���ݿ����",file->info.name );
						CloseHandle(fh);
						//ɾ����Ӧ�Ĵ����ļ�
						break;
					}
				}
				else {
					Shift sf = mm->GetView(ptr[index]);
					WriteFile(fh, sf.getEntry(), OriginalSize, NULL, NULL);
					mm->UnView(sf);
				}
			}
			CloseHandle(fh);
		}
	}

	void ReadFromMemory(PathNode* node, std::list<MemoryNode> * mlist) {

	}
////////////////////////////////New Writer//////////////////////////////////////////
	//���ⲿ����д��
	void Writer(std::string outBase, std::list<std::string> * list, PathNode * Node, bool AutoRenameAble = true) {
		std::list<ProcessNode> BatchList;
		std::list<ProcessNode> NormalList;
		std::list<ProcessNode> HugeList;
		for (auto e = list->begin(); e != list->end(); e++) {
			ProcessNode pn;
			pn.Info = getFileInfo(*e);
			pn.Outer = *e;
			if(pn.Info.data.count < Sets->BatchBlockSize)BatchList.push_back(pn);
			else if (pn.Info.data.count >= Sets->HugeFileSize)HugeList.push_back(pn);
			else NormalList.push_back(pn);
		}
		callback->OnWarring(0, "---��ʼ�ļ�д��---", "");
		BatchWrite(outBase,&BatchList,Node, AutoRenameAble);
		NormalWrite(outBase, &NormalList, Node, AutoRenameAble);
		HugeWrite(outBase, &HugeList, Node, AutoRenameAble);
		callback->OnWarring(0, "---�ļ�д�����---", "");
	}
	
	void HugeWrite(std::string outBase, std::list<ProcessNode> * HugeList, PathNode * TargetNode, bool AutoRenameAble) {
		std::list<SpaceNode> SList;

		for (auto e = HugeList->begin(); e != HugeList->end(); e++) {
			PathNode * node;
			std::string RelativePath = getRelativePath(outBase, e->Outer);
			if (AutoRenameAble && IsFileNodeExist(RelativePath, TargetNode)) {
				std::string OName = e->Info.name;
				AutoRename(&(e->Info), TargetNode);
				std::string NName = e->Outer.replace(e->Outer.rfind(OName), OName.size(), e->Info.name);
				node = MakeInnerTarget(NName, outBase, TargetNode);
			}
			else node = MakeInnerTarget(e->Outer, outBase, TargetNode);

			node->info = e->Info;
			node->info.type |= HUGE_TYPE;

			callback->OnWarring(0, "HugeWrite Target Path:", e->Outer);
			callback->OnWarring(0, "HugeWrite Target Node:", getAbsoulutePath(node));

			HANDLE fh = mm->OpenFile(e->Outer);
			LONGLONG TotalSize = 0;
			size_t TotalZipsize = 0;
			void* Buffer = malloc(Sets->SectionSize);
			
			void * ZipedData = NULL;
			size_t ZipedSize = 0;
			size_t index = 0;
			DWORD ReadedSize = 0; 
			
			SpaceNode sp;
			Shift sf;

			try {
				while (TotalSize < e->Info.data.count)
				{
					ReadFile(fh, Buffer, Sets->SectionSize, &ReadedSize, NULL);
					if (Sets->Compressable && IsCompressable(e->Outer) && ZipData(Buffer, ReadedSize,&ZipedData,&ZipedSize)) {
						//����ѹ�� ��ѹ�� ��ѹ���ɹ�
						sp = HoldSpace(ZipedSize);
						sf = mm->GetView(sp);
						memcpy(sf.getEntry(),ZipedData,ZipedSize);
						mm->UnView(sf);
						free(ZipedData);

						TotalZipsize += ZipedSize;
						node->info.zipedSize = TotalZipsize;//��¼��������ݵ��ܴ�С

					}
					else {
						sp = HoldSpace(ReadedSize);
						sf = mm->GetView(sp);
						memcpy(sf.getEntry(), Buffer, ReadedSize);
						mm->UnView(sf);
					}
					sp.blockindex = ReadedSize;//��¼ԭʼ���ݿ��С
					//�� sp.ccount = sp.blockindex ��ʾ���ݶ�û�б�ѹ������
					SList.push_back(sp);//ͨ���洢�Ŀ����е�˳�򰵺��ļ��ֶδ���
				
					TotalSize += ReadedSize;
				}
			}
			catch (int ErrorCode) {
				CloseHandle(fh);
				if (Buffer!=NULL)
				{
					free(Buffer);
					Buffer = NULL;
				}
				std::list<PathNode*> plist;
				plist.push_back(node);
				RemoveChildrens(&plist, &SList);
				//�Ի�δ��SpaceNode���У����Ѿ������˿ռ䣬��View�׶γ���Ŀռ�ڵ�����ͷ�
				if (ErrorCode == ERROR_VIEW_SEGMENT_OVERFLOW || ErrorCode == ERROR_TARGET_SIZE_OVERFLOW) {
					ReleaseSpace(sp);
				}
			}

			CloseHandle(fh);
			if (Buffer != NULL)
			{
				free(Buffer);
				Buffer = NULL;
			}

			SpaceNode hsp = SaveSpaceList(&SList);
			node->info.data.location = hsp.location;//�����д��λ��
			node->info.data.blockindex = TotalSize;
			node->info.data.count = SList.size();//����������
			node->pat->info.type |= INDEX_SUB_SIZECHANGED;
		}
	}
	
	void NormalWrite(std::string outBase, std::list<ProcessNode> * NormalList, PathNode * TargetNode, bool AutoRenameAble) {
		
		for (auto e = NormalList->begin(); e != NormalList->end(); e++) {
			PathNode * node;
			std::string RelativePath = getRelativePath(outBase, e->Outer);
			if (AutoRenameAble && IsFileNodeExist(RelativePath, TargetNode)) {
				std::string OName = e->Info.name;
				AutoRename(&(e->Info), TargetNode);
				std::string NName = e->Outer.replace(e->Outer.rfind(OName), OName.size(), e->Info.name);
				node = MakeInnerTarget(NName, outBase, TargetNode);
			}
			else node = MakeInnerTarget(e->Outer, outBase, TargetNode);
			node->info = e->Info;
			node->info.type |= NORMAL_TYPE | INDEX_SELF_MODIFY;

			callback->OnWarring(0, "NormalWrite Target Path:", e->Outer);
			callback->OnWarring(0, "NormalWrite Target Node:", getAbsoulutePath(node));


			HANDLE fh = mm->OpenFile(e->Outer);

			SpaceNode sp;
			Shift sf;

			try {
				if (Sets->Compressable && IsCompressable(e->Outer)){
					void * Buffer = malloc(e->Info.data.count);
					void * ZipedData = NULL;
					size_t ZipedSize = 0;
					ReadFile(fh, Buffer, e->Info.data.count, NULL, NULL);
					if (ZipData(Buffer, e->Info.data.count, &ZipedData, &ZipedSize)) {
						sp = HoldSpace(ZipedSize);
						sf = mm->GetView(sp);
						memcpy(sf.getEntry(), ZipedData, sp.count);
						mm->UnView(sf);
						free(ZipedData);
						free(Buffer);
						node->info.zipedSize = ZipedSize;
					}
					else {
						sp = HoldSpace(e->Info.data.count);
						sf = mm->GetView(sp);
						memcpy(sf.getEntry(), Buffer, sp.count);
						mm->UnView(sf);
						node->info.zipedSize = 0;
					}
				}
				else {
					sp = HoldSpace(e->Info.data.count);
					sf = mm->GetView(sp);
					ReadFile(fh, sf.getEntry(), e->Info.data.count, NULL, NULL);
					mm->UnView(sf);
					node->info.zipedSize = 0;
				}
			}
			catch (int ErrorCode) {
				CloseHandle(fh);
				RemoveChildren(node);
				if (ErrorCode == ERROR_VIEW_SEGMENT_OVERFLOW || ErrorCode == ERROR_TARGET_SIZE_OVERFLOW) {
					ReleaseSpace(sp);
				}
				callback->OnError(0,"Normal Writer Error ",getAbsoulutePath(node));
			}

			CloseHandle(fh);
			node->info.data.location = sp.location;

		}
	}
	
	void BatchWrite(std::string outBase, std::list<ProcessNode> * BatchList,PathNode * TargetNode, bool AutoRenameAble) {
		
		size_t Totalsize = 0,Index = 0;
		std::list<ProcessNode> List;
		std::list<SpaceNode> SList;
		std::list<PathNode*> RollBack;

		for (auto e = BatchList->begin(); e != BatchList->end(); e++ ) {
			Totalsize += e->Info.data.count;
			List.push_back(*e);
			if (Totalsize > Sets->BatchBlockSize || e == --BatchList->end()) {
				PathNode* bnode = GetUseableBlocknode(FindBatchNode(TargetNode));

				void * Buffer = malloc(Totalsize);
				size_t Offset = 0;
				DWORD WritedSize = 0;
				std::stringstream ss;
				for (auto i = List.begin(); i != List.end();i++) {
					//��ȡ����
					HANDLE hf = mm->OpenFile(i->Outer);
					
					ReadFile(hf, (BYTE*)Buffer + Offset, i->Info.data.count, &WritedSize, NULL);
					CloseHandle(hf);
					
					ss.str("");
					ss.clear();
					ss << i->Outer << " S: " << i->Info.data.count << " W: " << WritedSize << " O: "<< Offset <<" NT:"<< Offset + WritedSize;
					callback->OnWarring(0, "Batch ��ȡ���ݣ�" , ss.str());

					//����·���ڵ�
					PathNode * node;
					callback->OnWarring(0, "BatchWrite Target Path:", getRelativePath(outBase, i->Outer));
					std::string RelativePath = getRelativePath(outBase, i->Outer);
					callback->OnWarring(0, "BatchWrite Target Path:", i->Outer);

					if (AutoRenameAble && IsFileNodeExist(RelativePath, TargetNode)) {
						std::string OName = i->Info.name;
						AutoRename(&(i->Info), TargetNode);
						std::string NName = i->Outer.replace(i->Outer.rfind(OName), OName.size(),i->Info.name );
						callback->OnWarring(0, "BatchWrite Target newName:", NName);
						node = MakeInnerTarget(NName, outBase, TargetNode);	
					}
					else node = MakeInnerTarget(i->Outer, outBase, TargetNode);
					
					callback->OnWarring(0, "BatchWrite Target Path:", i->Outer);
					callback->OnWarring(0, "BatchWrite Target Node:", getAbsoulutePath(node));
					
					//�����ļ��ڵ���Ϣ
					PathNode file;
					file.info = i->Info;
					file.info.data.blockindex = bnode->info.data.blockindex;//Batch���ݿ��Index
					file.info.data.location = Index;//ƫ�Ƶ�Index
					file.info.data.count = i->Info.data.count;
					file.info.type |= BATCH_TYPE | INDEX_SELF_MODIFY;
					node->addChild(&file);
					
					//�����ļ�����д��ʧ��ʱ���г���
					RollBack.push_back(&node->sub->back());

					//ƫ�ƽڵ�
					SpaceNode sp;
					sp.blockindex = Index;//ƫ�����
					sp.location = Offset;//ƫ����
					sp.count = i->Info.data.count;//���ݴ�С
					SList.push_back(sp);
					
					std::cout << " Index :" << Index << " Location:" << Offset << " Count:" << sp.count << " NT:"<< Offset+sp.count << " BIndex:" << bnode->info.data.blockindex << std::endl;

					//����ƫ��
					Offset += i->Info.data.count;
					Index++;
				}
				List.clear();

				size_t ZipSize = 0;
				void * ZipdData = NULL;
				SpaceNode sp;
				Shift sf;

				try {
					//д������
					//������ѹ�����ã�����ѹ���ɹ�
					if (Sets->Compressable && ZipData(Buffer, Totalsize, &ZipdData, &ZipSize)) {
						bnode->info.zipedSize = ZipSize;
						sp = HoldSpace(ZipSize);
						sf =  mm->GetView(sp);
						memcpy(sf.getEntry(), ZipdData, ZipSize);
						mm->UnView(sf);
						free(ZipdData);
						ZipdData = NULL;
					
					}
					else {//δ����ѹ��������ѹ��ʧ��
						bnode->info.zipedSize = 0;
						sp = HoldSpace(Totalsize);
						sf = mm->GetView(sp);
						memcpy(sf.getEntry(), Buffer, Totalsize);
						mm->UnView(sf);
					}

				}
				catch (int ErrorCode) {
					if (Buffer != NULL) {
						free(Buffer);
						Buffer = NULL;
					}
					RemoveChildrens(&RollBack, &SList);
					if (ErrorCode == ERROR_VIEW_SEGMENT_OVERFLOW || ErrorCode == ERROR_TARGET_SIZE_OVERFLOW) {
						ReleaseSpace(sp);
						callback->OnError(2003,"BatchBlockѹ��ʧ��","");
					}
				}

				if (Buffer != NULL) {
					free(Buffer);
					Buffer = NULL;
				}

				//Batch�ڵ���Ϣ
				bnode->info.data.count = Totalsize;
				bnode->info.data.location = sp.location;
				bnode->info.type |= INDEX_SELF_MODIFY;
				//ƫ���б���Ϣ
				SpaceNode hsp = SaveSpaceList(&SList);
				SList.clear();
				LONGLONG * NP = (LONGLONG *)bnode->info.name;
				NP[SPL_START] = hsp.location;
				NP[SPL_SIZE] = hsp.count;
				//std::cout << " B-NODE :" << bnode->info.data.blockindex << " Location:" << hsp.location << " Count:" << hsp.count << " ListSize:"<< hsp.count/sizeof(SpaceNode) <<std::endl;
				
				Totalsize = 0;
				Index = 0;
			}
			RollBack.clear();
		}
	}

	void WriteFromMemory(std::list<MemoryNode> * mlist, std::string outBase) {

	}
/////////////////////////////////Error Handle /////////////////////////////////////////
	void RemoveChildrens(std::list<PathNode*> * Roollback, std::list<SpaceNode>* splist) {
		for (auto e : *Roollback) {
			if (isBatchFile(e)) {
				
			}
			else if (isNormalFile(e)) {
				

			}
			else if (isHugeFile(e)) {
				if(splist!=NULL)for (auto sp : *splist) {
					ReleaseSpace(sp);
				}
			}
			else
			{
				std::stringstream ss;
				ss << " �ͷŽڵ� " <<getAbsoulutePath(e) << e->info.name << " ���� " << e->info.type;
				callback->OnWarring(0, "δ֪��ǣ��ع�����ʧ��", ss.str());
			}
			e->pat->sub->remove(*e);
		}
	}
	
	void RemoveChildren(PathNode * node) {
		if(node->pat != NULL && node->pat->sub != NULL)
			node->pat->sub->remove(*node);
	}

////////////////////////////////Attach Process//////////////////////////////////////////

	//�����ⲿ��׼·�����ڲ������ļ��ڵ�ĸ��ڵ㣬���ļ��ڵ㱾���Դ����ⲿ·�����ļ�����
	HANDLE MakeOutTarget(std::string OutSaveBase, PathNode * InnnerBase, PathNode * TargetFile, bool OverWrite = false) {
		//�����ⲿ·��
		std::string relative = getRelativePath(InnnerBase, TargetFile->pat);
		if (relative.find(".\\") == 0 || relative.find("\\.\\") != relative.npos) {
			relative = ReplaceAll(relative, ".\\", "");
			callback->OnWarring(9989, "�����ⲿ·��ʱ����·������δ�����͵����·������ǿ��ת��·��", relative);
		}
		else
		{
			if (!EndWith(OutSaveBase, "\\"))relative.insert(0, "\\");
		}
		//�ϳ��ⲿָ��Ŀ���ļ������ļ��е�·��
		std::string outPath = OutSaveBase + relative;
		if (!EndWith(outPath, "\\"))outPath.append("\\");
		int ErrorCode = MakeOutDirs(outPath, outPath);
		callback->OnWarring(ErrorCode, "�����ⲿ·��:", outPath);
		std::string FileName;
		if (TargetFile->info.type & LONG_NAME) {
			FileName = *GetLongName(TargetFile);
		}
		else FileName = TargetFile->info.name;

		HANDLE fh = mm->NewFile(outPath + FileName);
		//�����ⲿ�ļ�
		if (OverWrite && fh == NULL) {
			fh = mm->OpenFile(outPath + TargetFile->info.name);
			callback->OnWarring(909, "�������ⲿ�ļ�", outPath + TargetFile->info.name);
		}
		if (!OverWrite && fh == NULL) {
			callback->OnError(909, "�ⲿ�ļ��Ѵ��ڣ���δ�����ǣ�д���ļ�����ʧ��", TargetFile->info.name);
		}
		return fh;
	}
	//����Ŀ��ڵ�·������·�������ص���ʼ�ڵ㣬����·��������·���ڵ㼰�ļ��ڵ�
	PathNode * MakeInnerTarget(std::string Path ,std::string Base, PathNode* BaseNode) {
		std::string r_path = getRelativePath(Base, Path);
		
		if (r_path.find_first_of("\\") != r_path.npos) {
			r_path = getPathFromFilePath(r_path);
		}
		callback->OnError(0,"MakeInnerTarget ",r_path );
		return MakeDirs(BaseNode, r_path);//����Ŀ¼
	}

	bool UnZipData(void * src, size_t srcSize, void ** dst, size_t *dstSize, int CompressLevel = 8) {
		ULONG dsize=*dstSize, ssize=srcSize;
		*dst = malloc(*dstSize);
		int ErrorCode = uncompress2((BYTE*)*dst, &dsize, (BYTE*)src, &ssize);
		if (ErrorCode != Z_OK) {
			std::stringstream ss;
			ss << "ԭʼ���ݴ�С:" << ConvertSpaceToRead(dsize);
			callback->OnError(ErrorCode,"��ѹѹ�����ݿ����", ss.str());
			free(*dst);
			return false;
		}
		//*dstSize = dstSize;
		return true;
	}

	bool ZipData(void * src,size_t srcSize,void ** dst,size_t *dstSize,int CompressLevel = 8) {
		ULONG zipsize = compressBound(srcSize);
		*dst = malloc(zipsize);
		int ErrorCode = compress2((BYTE*)*dst, &zipsize, (BYTE*)src, srcSize, CompressLevel);
		*dstSize = zipsize;
		if (ErrorCode != Z_OK) {
			std::stringstream ss;
			ss << "���ݴ�С:" << ConvertSpaceToRead(srcSize);
			callback->OnError(ErrorCode,"ѹ�����ݿ����", ss.str());
			free(*dst);
			*dst = NULL;
			return false;
		}
		return true;
	}

	SpaceNode SaveSpaceList(std::list<SpaceNode>* NList) {
		if (NList == NULL || NList->size() == 0)return SpaceNode{0,0,0};
		size_t size = NList->size() * sizeof(SpaceNode);

		SpaceNode sp = HoldSpace(size);
		
		Shift sf = mm->GetView(sp);
		size_t Offset = 0;

		for (auto e = NList->begin(); e != NList->end(); e++) {
			memcpy(sf.getEntry() + Offset, &(*e), sizeof(SpaceNode));
			Offset += sizeof(SpaceNode);
		}
		mm->UnView(sf);
		std::stringstream ss;
		ss << " L:" << sp.location << " C:" << sp.count << " NT:" << sp.location + sp.count;
		callback->OnWarring(0, "д��ƫ���� ",ss.str() );
		return sp;
	}

	void LoadSpaceList(SpaceNode SP, std::list<SpaceNode>* NList) {
		Shift sf = mm->GetView(SP);
		SpaceNode* ptr = (SpaceNode*)sf.getEntry();
		size_t size = SP.count / sizeof(SpaceNode);
		for (size_t index = 0; index < size; index++) {

			NList->push_back(ptr[index]);
		}
		mm->UnView(sf);
	}

	bool IsCompressable(std::string name) {
		std::string type = getFileType(name);
		if (type.compare("Unknown")) {
			callback->OnWarring(0, name.append(" ��δ֪�ļ����ͣ�����ʹ��Zipѹ������"), name);
			return true;
		}
		if (NOZipTarget.find(getFileType(type)) == NOZipTarget.npos)return true;
		return false;
	}

//////////////////////////////////////////////////////////////////////////

	//��ȡBatch�ڵ��¿��õ�batch�飬û���򴴽��½ڵ㡣������д��Batch�ļ�
	PathNode * GetUseableBlocknode(PathNode * node) {
		if (node->info.type & FILE_TYPE)return NULL;
		if (node->sub == NULL)node->sub = new std::list<PathNode>();

		if (node->sub->size() == 0) {//��ǰĿ¼Ϊ�գ�ֱ�Ӵ���Batch�ڵ�
			PathNode ps;
			ps.info.type = BATCH_BLOCK | FILE_TYPE | INDEX_SELF_MODIFY;
			ps.info.data.blockindex = 0;
			ps.info.setName(std::string("BATCHBLOCK_").append(ConvertToString(time(0))));
			ps.pat = node;

			node->info.type |= INDEX_SUB_SIZECHANGED;
			node->addChild(&ps);
			return &node->sub->back();
		}
		else {
			LONGLONG index = 0;
			std::list<PathNode>::iterator it = node->sub->begin(), sit;
			while (it != node->sub->end())//������Ч��BatchFileIndex
			{
				index++;
				it++;
			}
			
			PathNode ps;
			ps.info.type = BATCH_BLOCK | FILE_TYPE | INDEX_SELF_MODIFY;
			ps.info.data.blockindex = index;
			ps.info.setName(std::string("BATCHBLOCK_").append(ConvertToString(time(0))));
			ps.pat = node;

			node->info.type |= INDEX_SUB_SIZECHANGED;
			node->addChild(&ps);
			return &node->sub->back();

		}
	}

	//��ȡ��Ŀ¼�µ�Batch�ڵ�,�����ڶ�ȡBatch�ļ�,û�о��½�
	PathNode * FindBatchNode(PathNode * node) {

		PathNode * p_node = node;
		while (!isPartition(p_node)) {
			p_node = p_node->pat;
		}

		std::list<PathNode>::iterator it = p_node->sub->begin();

		if (p_node->sub == NULL)goto CREATENEW;
		if (p_node->sub->size() == 0)goto CREATENEW;


		while (it != p_node->sub->end()) {
			if (it->info.type & BATCH_TYPE)return &(*it);
			it++;
		}
	CREATENEW:
		PathNode ps;
		ps.info.type = BATCH_TYPE| FLODER_TYPE | INDEX_SELF_MODIFY;
		std::string bname = "BatchNode_";
		bname.append(ConvertToString(time(0)));
		ps.info.setName(bname);
		ps.pat = node;
		ps.sub = new std::list<PathNode>();

		p_node->sub->push_back(ps);
		p_node->info.type |= INDEX_SUB_SIZECHANGED;
		return &p_node->sub->back();
	}

	//��ȡָ��index��Batch��ڵ�
	PathNode * GetTargetBatchNode(PathNode * batchnode, size_t targetindex) {
		std::list<PathNode>::iterator it = batchnode->sub->begin();
		while (it != batchnode->sub->end()) {
			if (it->info.data.blockindex == targetindex) {
				return &(*it);
			}
			it++;
		}
		return NULL;
	}

	//���ڵ㰴BatchIndex����,���������нڵ�����ݴ�С,��BatchNode�ļ�д��BatchIndexΪ-1(Ĭ��)
	size_t GroupBatchNodes(std::list<PathNode*> * list , std::map<LONGLONG, std::list<PathNode *>> * kv) {
		size_t totalsize = 0 ;
		LONGLONG index = 0;
		for (auto e = list->begin(); e != list->end(); e++) {

			totalsize += (*e)->info.data.count;
			PathNode * ps = *e;
			
			auto p = kv->find(ps->info.data.blockindex);
			if (p != kv->end()) {//�ҵ�
				p->second.push_back(ps);
			}
			else {
				auto r = kv->insert(std::make_pair(ps->info.data.blockindex, std::list<PathNode*>()));
				//p = kv.find(ps->info.data.blockindex);
				r.first->second.push_back(ps);
			}
		}
		return totalsize;
	}
	
	//�ҵ�ָ����ŵ�Sequence�ֿ����ݽڵ�
	PathNode * getTargetSequenceNode(PathNode * filenode, size_t index) {
		for (auto e = filenode->sub->begin(); e != filenode->sub->end(); e++) {
			if (e->info.data.blockindex == index)return &(*e);
		}
		callback->OnWarring(9907, "�޷��ҵ�ָ����SeqNode", "");
		return NULL;
	}
///////////////////////////////�ļ���д//////////////////////////////////////////

};

/*
	ULONG ZipData(FileInfo * fi, Shift sp,int level = 7) {
		ULONG c_size = compressBound((ULONG)fi->data.count);
		fi->zipedSize = c_size;
		BYTE * dataPtr = (BYTE*)malloc(c_size);
		compress2((BYTE*)sp.ptr+sp.offset,&c_size,dataPtr, (ULONG)fi->data.count,level);
		return c_size;
	}

	ULONG ZipData(BYTE* in_ptr, BYTE* out_ptr, ULONG dataSize,int level =7) {
		ULONG c_size = compressBound(dataSize);
		if (compress2(out_ptr, &c_size, in_ptr, dataSize, level) != Z_OK)
			std::cout << " compress error" << std::endl;
		return c_size;
	}

	void UnZipData(Shift sp,FileInfo fi,void * outPtr) {
		//ѹ�������ݴ�С
		ULONG c_size = (ULONG)fi.zipedSize;
		//ԭʼ���ݴ�С
		ULONG o_size = (ULONG)fi.data.count;
		uncompress2((BYTE*)outPtr,&o_size,(BYTE*)sp.ptr+sp.offset,&c_size);
	}

	void UnZipData(BYTE* in_ptr, BYTE* out_ptr, ULONG dataSize, ULONG originalSize) {
		if (uncompress2(out_ptr, &originalSize, in_ptr, &dataSize) != Z_OK)
			std::cout << " Uncompress Failed " << std::endl;
	}
	
	

	ULONG _ZipBatchWriteIn(BYTE * data_ptr, ULONG data_size, Shift buffer_struct,ULONG buffer_size, int level = 7) {
		BYTE * out_ptr = buffer_struct.getEntry();
		int code = compress2(out_ptr, &buffer_size , data_ptr, data_size, level);
		if (code != Z_OK) {
			//std::cout << " Batch Zip Error " << std::endl;
		}
		return buffer_size;
	}
	
	void _UnZipBatchReadOut(Shift st,FileInfo batch_info,BYTE * out_data_buffer) {
		BYTE* data_ptr = st.getEntry();
		ULONG original_size = (ULONG)batch_info.data.count;
		ULONG ziped_size = batch_info.zipedSize;
		int code = uncompress2(out_data_buffer, &original_size, data_ptr, &ziped_size);
		if (code != Z_OK) {
			std::cout << " Batch Unzip Error " << std::endl;
		}

	}

	//��ָ���ⲿ�ļ�����д��ڵ�ͬĿ¼�µ�Batch���ݿ��У������ں���ʵ��������ȡ�����������ļ����е��ļ���
	void BatchWriteIn(std::string outerFloder,std::list<std::string> * srcs, PathNode * node) {//�����洢����������ļ�
		std::list<FileInfo> fl;
		std::list<std::string> sl;
		size_t totalsize = 0;
		unsigned int blockIndex = 0;

		while (srcs->size()>0) {

			FileInfo fi = getFileInfo(srcs->front(), INDEX_SELF_MODIFY);
			
			if (fi.data.count >= Sets->BatchBlockSize) {//����������������д��
				WriteIn(srcs->front(), node);
				srcs->pop_front();
			}
			else {//�ɽ���������

				fl.push_back(fi);
				sl.push_back(srcs->front());
				srcs->pop_front();
				totalsize += fi.data.count;
				
				//�����ݴﵽ���С���� ���� �Ѿ������������ļ����� �㿪ʼ����д���
				if (totalsize >= Sets->BatchBlockSize || srcs->size() == 0) {

					PathNode * B_node = getUseableBlocknode(findBatchNode(node));//��ȡ����BATCH�ڵ㣬û���򴴽�

					size_t file_offset = 0;

					if (Sets->Compressable) {
						//Buffer1 ԭʼ�ļ����ݼ���
						//Buffer2 ѹ�����ļ����ݼ���
						//Shift ��������ָ���Ŀ��ṹ��
						//
						 //���ļ����ݶ�ȡ���ڴ棬��������Ӧ�ڵ��ƫ����Ϣ
						// ѹ�����ݵ�ѹ�����棬����ȡʵ��ѹ����Ĵ�С
						// ��ѹ���������д��ӳ���ļ�ָ��λ�ã�������Batch�ڵ�
						
						ULONG ZipedSize = compressBound((ULONG)totalsize);
						BYTE* Buffer1 = (BYTE*)malloc(totalsize);
						BYTE* Buffer2 = (BYTE*)malloc(ZipedSize);
						SpaceNode sp = HoldSpace(ZipedSize);
						ReleaseSpace(sp);
						if (sp.blockindex == -1) {//�ֿ�洢
							std::cout << " When Batch Write , Need Sequence Write " << std::endl;
						}

						while (fl.size()>0)
						{
							fl.front().data.blockindex = B_node->info.data.blockindex;
							fl.front().data.location = file_offset;
							fl.front().type |= BATCH_BLOCK;

							std::string r_path = getRelativePath(outerFloder, sl.front());
							r_path = getPathFromFilePath(r_path);
							PathNode * p_node = getNodeByPath(node, r_path);
							//��ƫ�Ƶ�д������ ��node�¶�Ӧ·������ӽڵ�
							WriteIn(sl.front(), &fl.front(), p_node, Buffer1 + file_offset);
							file_offset += fl.front().data.count;

							fl.pop_front();
							sl.pop_front();
						}
						
						Shift sf;
						sf.ptr = Buffer2;
						sf.offset= 0;
						LONG real_Size = _ZipBatchWriteIn(Buffer1, (ULONG)totalsize, sf, ZipedSize);
						free(Buffer1);
						B_node->info.data.location = sp.location;
						B_node->info.data.count = totalsize;
						B_node->info.zipedSize = real_Size;
						//std::cout << "Batch Zip "<< sp.location<<" @ "<< real_Size<<" from "<< totalsize<< std::endl;
						sp = HoldSpace(ZipedSize);
						sf= mm->GetView(sp);
						memcpy(sf.getEntry(),Buffer2,real_Size);
						free(Buffer2);
						mm->UnView(sf);
						
						totalsize = 0;

						std::stringstream sss;
						sss << " Batchд�룺" << getAbsoulutePath(B_node) << " F: " << sp.location << " T: " << sp.location + real_Size ;
						callback->OnWarring(sss.str(), 0);
					}
					else {

						SpaceNode sp = HoldSpace(totalsize);
						Shift  sptr = mm->GetView(sp);

						B_node->info.data.location = sp.location;
						B_node->info.data.count = sp.count;

						while (fl.size() > 0) {
							fl.front().data.blockindex = B_node->info.data.blockindex;
							fl.front().data.location = file_offset;
							fl.front().type |= BATCH_BLOCK;
							//std::cout << "ƫ�Ƶ�ַ��" << file_offset << "\t �ļ���С��"<< fl.front().data.count<<"\t ";
					
							std::string r_path = getRelativePath(outerFloder, sl.front());
							r_path =getPathFromFilePath(r_path);
							PathNode * p_node = getNodeByPath(node,r_path);
							//��ƫ�Ƶ�д������ ��node�¶�Ӧ·������ӽڵ�
							WriteIn(sl.front(), &fl.front(), p_node, (BYTE*)sptr.ptr + sptr.offset + file_offset);
							file_offset += fl.front().data.count;
					
							fl.pop_front();
							sl.pop_front();
						}
						mm->UnView(sptr);
						totalsize = 0;
					}
					
				}
			
			}
			
		}
		node->info.type |= INDEX_SUB_SIZECHANGED;

	}

	//��node�ڵ���ѡ���Ķ��������ļ���������������ӽڵ㣩 ���뵽�ڴ��С�ͨ��ptr�����룬�ɱ��������ٿռ䲢�������
	void BatchLoadToMemory(PathNode* node , std::list<PathNode*> * targetList , std::list<MemoryNode> * result) {
		
		std::map<LONGLONG, std::list<PathNode *>> kv;
		
		size_t totalsize = GroupBatchNodes(targetList, &kv);
		void * ptr = malloc(totalsize);
		size_t offset = 0;

		PathNode * batch_node = findBatchNode(node);

		ULONG zip_size, original_size;
		//��BatchIndex���鴦���ļ�
		for (auto e = kv.begin(); e != kv.end(); e++) {
			PathNode * batch_file_node = getTargetBatchNode(batch_node, e->first);

			if (batch_file_node->info.zipedSize > 0) {//��ǰBatchBlock��ѹ��
				
				SpaceNode sp = batch_file_node->info.data;
				sp.count = batch_file_node->info.zipedSize;
				Shift sf = mm->GetView(sp);

				zip_size = (ULONG)sp.count;
				original_size = (ULONG)batch_file_node->info.data.count;

				void * buffer_ptr = malloc(original_size);
				if (uncompress2((BYTE*)buffer_ptr, &original_size, sf.getEntry(), &zip_size) != Z_OK)
					std::cout << " Batch Unzip Error " << std::endl;
				
				mm->UnView(sf);
				
				size_t wrtied_size = 0;
				for (auto i = e->second.begin(); i != e->second.end(); i++) {
					MemoryNode mn;
					mn.name = getRelativePath(node, *i);
					mn.size = (*i)->info.data.count;
					mn.offset = (*i)->info.data.location;
					mn.ptr = ptr;

					memcpy((BYTE*)ptr + offset + wrtied_size, (BYTE*)buffer_ptr + mn.offset, mn.size);
					mn.offset = offset + wrtied_size;
					result->push_back(mn);

					wrtied_size += mn.size;
				}
				free(buffer_ptr);
				offset += wrtied_size;
			}
			else {//δѹ����batch��ֱ���ڴ渴��
				SpaceNode sp = batch_file_node->info.data;
				Shift sf = mm->GetView(sp);

				for (auto i = e->second.begin(); i != e->second.end(); i++) {
					MemoryNode mn;
					mn.name = getRelativePath(node, *i);
					mn.size = (*i)->info.data.count;
					mn.ptr = ptr;
					
					memcpy((BYTE*)ptr + offset, sf.getEntry()+ (*i)->info.data.location , mn.size);
					mn.offset = offset;
					offset += mn.size;
					//���ļ���Ϣ�����б�
					result->push_back(mn);
				}
				mm->UnView(sf);
				
			}
			
			
		}

	}
	
	
	//���BatchNode��ÿ���ļ����ⲿ�ľ���·��(�������ļ���);
	std::string getBatchOutPath(std::string outBase,PathNode * node,PathNode * e) {
		//������ͬ���ļ��нṹ
		std::string r_path;
		r_path = getRelativePath(node, e);
		size_t dot = r_path.find_last_of(".");
		if (dot != r_path.npos) {//·���������ļ�����β����Ҫ��ȡ��·��
			size_t spl = r_path.find_last_of("\\");
			if (spl != r_path.npos) {
				if (dot > spl) {//���ļ�·��
					r_path = r_path.substr(0, spl);
				}
			}
			else {//·����û���ļ��У�·��Ϊ���ļ���
				r_path = "";
			}
		}
		return r_path.insert(0, outBase);
	}

	//���ڵ��µ�Batch���ݿ���ļ����� ����д�뵽�����ļ�
	void BatchReadOut(std::list<PathNode *> *nodeList, std::string outPath, PathNode * node) {
		std::map<LONGLONG, std::list<PathNode *>> kv;

		PathNode * b_node = findBatchNode(node);

		if (b_node == NULL || b_node->sub == NULL || b_node->sub->size() == 0) {
			//Ŀ¼�²�������Batch��ʽд����ļ�����
			return;
		}
		//��blockindex��list����
		GroupBatchNodes(nodeList, &kv);
		
		//һ��һ��ؽ�����д���ļ�
		for (auto i = kv.begin(); i != kv.end(); i++) {
			
			PathNode * n = getTargetBatchNode(b_node, i->first);
			size_t file_offset = 0;

			if (n->info.zipedSize > 0) {//�鱻ѹ������Ҫ��ѹ
				SpaceNode sp;
				sp.count = n->info.zipedSize;
				sp.location = n->info.data.location;
				std::cout << "Batch Unzip " << sp.location << " @ " << sp.count <<" from "<< n->info.data.count<< std::endl;
				
				Shift sf = mm->GetView(sp);

				BYTE * data_ptr = (BYTE*)malloc(n->info.data.count);
				_UnZipBatchReadOut(sf, n->info, data_ptr);

				for (auto e = i->second.begin(); e != i->second.end(); e++) {
					file_offset = (*e)->info.data.location;

					//������ͬ���ļ��нṹ
					std::string r_path = getBatchOutPath(outPath, node, *e);

					//��ƫ�Ƶ�д������
					ReadOut(r_path, &(*e)->info, data_ptr  + file_offset);
				}

				free(data_ptr);
				mm->UnView(sf);
			}
			else {
				Shift sptr = mm->GetView(n->info.data);
			
				for (auto e = i->second.begin(); e != i->second.end(); e++) {
					file_offset = (*e)->info.data.location;
				
					//������ͬ���ļ��нṹ
					std::string r_path = getBatchOutPath(outPath, node, *e);
				
					//��ƫ�Ƶ�д������
					ReadOut(r_path, &(*e)->info, (BYTE *)sptr.ptr + sptr.offset + file_offset);
				}
				mm->UnView(sptr);
		
			}

		}

	}

	
	//���ļ�����д��ָ��λ�ã����ڸ��ڵ��´�����Ӧ�ڵ� ���ڴ�ƫ��
	void WriteIn(std::string outPath, FileInfo * info, PathNode * node, void * ptr) {
		size_t readed = 0, r = info->data.count / Sets->SectionSize;
		DWORD tmp = 0;
		size_t size = 0;

		HANDLE hd = mm->OpenFile((char*)outPath.c_str());
		//std::cout << "�ڵ㣺" << node->info.name << "\t д���ļ���" << info->name<<std::endl;

		for (size_t i = 0; i <= r; i++) {//����Ҫд��1��
			size = info->data.count - readed;

			ReadFile(hd, (BYTE *)ptr + i * Sets->SectionSize, (DWORD)(size > Sets->SectionSize ? Sets->SectionSize : size), &tmp, NULL);
			readed += tmp;
		}
		CloseHandle(hd);
		PathNode newNode;
		newNode.info = *info;
		newNode.pat = node;
		node->sub->push_back(newNode);
		node->info.type |= INDEX_SUB_SIZECHANGED;
	}
	
	//��ָ���ڵ��µ��ļ�����д��ָ���ļ��У����������ļ����ļ��� ��ӳ��ƫ��
	void ReadOut(std::string outPath, FileInfo * info, void * ptr) {
		if (!EndWith(outPath, "\\"))outPath.append("\\");
		MakeOutDirs(outPath, outPath);
		outPath.append(info->name);
		HANDLE hd = mm->OpenFile((char *)outPath.c_str(), OPEN_ALWAYS);
		size_t writed = 0, r = info->data.count / Sets->SectionSize;
		DWORD tmp;
		size_t towrite;
		for (size_t i = 0; i <= r; i++) {
			towrite = info->data.count - writed;

			WriteFile(hd, (BYTE *)ptr + i * Sets->SectionSize,(DWORD)( towrite > Sets->SectionSize ? Sets->SectionSize : towrite), &tmp, NULL);
			writed += tmp;
		}
		CloseHandle(hd);
	}
	
	//��һ���ļ����ݸ��Ƶ��ڴ���
	void ReadOutToMemory(PathNode * node , MemoryNode * mn) {
		if (node->info.type & FILE_TYPE) {
			if (node->info.type & SEQUENCE_BLOCK) {
				if (node->sub == NULL) {
					LoadIndex(node);
				}
				BYTE * ptr = (BYTE *)malloc(node->info.data.count);
				size_t offset = 0;
				for (auto i = node->sub->begin(); i != node->sub->end(); i++) {
					Shift sp = mm->GetView(i->info.data);
					memcpy(ptr + offset, (BYTE *)sp.ptr + sp.offset, i->info.data.count);
					mm->UnView(sp);
					offset += i->info.data.count;
				}
				
				mn->name = node->info.name;
				mn->size = node->info.data.count;
				mn->offset = 0;
				mn->ptr = ptr;
				
			}
			else if (node->info.type & BATCH_BLOCK) {
				PathNode* b_root = findBatchNode(node);
				PathNode * b_d_node = getTargetBatchNode(b_root, node->info.data.blockindex);
				if (b_d_node->info.zipedSize > 0) {//ѹ����
					
					SpaceNode sp_data = b_d_node->info.data;
					ULONG o_size = (ULONG)sp_data.count, z_size = b_d_node->info.zipedSize;
					void * ptr = malloc(sp_data.count);
					sp_data.count = b_d_node->info.zipedSize;
					Shift sf = mm->GetView(sp_data);
					
					if (uncompress2((BYTE*)ptr, &o_size, sf.getEntry(), &z_size) != Z_OK) {
						std::cout << " Unzip Signed Target From Batch Node Error " << std::endl;
					}
					mm->UnView(sf);
					void * p_dat = malloc(node->info.data.count);
					memcpy(p_dat, (BYTE*)ptr + node->info.data.location, node->info.data.count);
					
					free(ptr);

					mn->ptr = p_dat;
					mn->offset = 0;
					mn->size = node->info.data.count;
					mn->name = node->info.name;
				}
				else {//δѹ��
					Shift sf = mm->GetView(b_d_node->info.data);
					void *  p_dat = malloc(node->info.data.count);
					memcpy(p_dat, sf.getEntry()+ node->info.data.location, node->info.data.count);
					mm->UnView(sf);
					mn->ptr = p_dat;
					mn->offset = 0;
					mn->size = node->info.data.count;
					mn->name = node->info.name;
				}
			}
			else{
				if (node->info.zipedSize == ZIP_INNER) {
					Shift sf = mm->GetView(node->info.data);
					void * ptr =NULL;
					size_t real_size = _ZipSectionReadOutToMemory(sf,&ptr);
					mm->UnView(sf);
					
					mn->name = node->info.name;
					mn->size = real_size;
					mn->offset = 0;
					mn->ptr = ptr;
				}
				else {
					Shift sp = mm->GetView(node->info.data);
					BYTE * ptr = (BYTE *)malloc(node->info.data.count);
					memcpy(ptr, (BYTE *)sp.ptr + sp.offset, node->info.data.count);
					mm->UnView(sp);
					
					mn->name = node->info.name;
					mn->size = node->info.data.count;
					mn->offset = 0;
					mn->ptr = ptr;
				}
				
			}
		}
	}

	void EmptyBlockData(PathNode * b_d_node,SpaceNode data) {
		Shift sp = mm->GetView(b_d_node->info.data);
		memset((BYTE*)sp.ptr + sp.offset + data.location, 0, data.count);
	}

	void SaveMemoryToFile(std::string outPath ,MemoryNode * mnode) {
		std::string path = mnode->name;
		if (StartWith(path, "\\") && StartWith(outPath, "\\"))path = path.substr(1);
		if (!StartWith(path, "\\") && !StartWith(outPath, "\\"))path.insert(0, "\\");
		path = outPath.append(path);
		MakeOutDirs(path,path);
		HANDLE hd = mm->OpenFile((char*)path.c_str(), OPEN_ALWAYS);
		WriteFile(hd,(BYTE*)mnode->ptr+mnode->offset,(DWORD)mnode->size,NULL,NULL);
		CloseHandle(hd);
	}
	//�ͷ�nodeָ����ڴ滺�棬
	//�����node��ͨ��BatchLoadToMemory��õĽڵ�֮һ�������ͷŽ�����ͬһ��BatchNode�س�������MemoryNodeʧЧ
	void ReleaseMemoryNode(MemoryNode * node) {
		free(node->ptr);
	}
	//�ͷ�ͨ��BatchLoadToMemory��õĽڵ����нڵ㣬�����list
	void ReleaseBatchMemoryNode(std::list<MemoryNode> * list) {
		std::map<void *, MemoryNode *> kv;
		for (auto i = list->begin(); i != list->end(); i++) {
			auto t = kv.find(i->ptr);
			if (t != kv.end()) {
				t->second = &(*i);
			}
			else {
				
				kv.insert(std::make_pair(i->ptr, &(*i)));
			}
		}
		for (auto k = kv.begin(); k != kv.end(); k++) {
			free(k->first);
		}
		list->clear();
	}


	//��ָ�����ⲿ�ļ�����д��ָ���ڵ��µ��µ��ļ��ڵ�
	bool WriteIn(std::string outPath, PathNode * node, bool autorename = false) {
		FileInfo fi = getFileInfo(outPath);
		HANDLE hd = mm->OpenFile((char *)outPath.c_str(), OPEN_EXISTING);

		if (autorename) AutoRename(&fi, node);

		PathNode newNode;
		SpaceNode sp = HoldSpace(fi.data.count);

		if (sp.blockindex == -1) {//�ֿ�洢,��ģʽ�²���ʹ��ѹ���洢��
			//����ѹ���洢��Ҫ֪���ֿ���������Ҫ��һ���������ž���ѹ��Ч��
			fi.type |= SEQUENCE_BLOCK;

			size_t size = fi.data.count - sp.count;
			int blockindex = 0;

			newNode.sub = new std::list<PathNode>();

			while (true) {
				Shift sptr = mm->GetView(sp);
				if (!_SectionWriteIn(hd, sp.count, sptr)) { //�������д�����������Ѿ�ռ�õĽڵ��ͷ�
					//����д�벻һ��
					for (size_t i = 0; i < newNode.sub->size(); i++) {
						PathNode e = newNode.sub->front();
						ReleaseSpace(e.info.data);
						newNode.sub->pop_front();
					}
					newNode.sub->clear();
					mm->UnView(sptr);
					return false;
				}
				mm->UnView(sptr);
				//����Sequence�ڵ㣬��¼���ݿ�洢��Ϣ
				PathNode ps;
				ps.info.data = sp;
				ps.info.data.blockindex = blockindex;
				ps.info.type = SEQUENCE_BLOCK;

				newNode.sub->push_back(ps);

				//��һ�����ݿ����
				blockindex++;

				if (size <= 0) {//�����Ѿ�д�����˳�ѭ��
					break;
				}
				//����δд�꣬�������¿ռ䣬������ʣ�����ݴ�С
				sp = HoldSpace(size);
				size = size - sp.count;
			}
			//�����ļ��ڵ���Ϣ
			newNode.info = fi;
		}
		else {//ֱ�Ӵ洢

			if (Sets->Compressable) {
				ReleaseSpace(sp);

				ULONG r = (ULONG)(fi.data.count / Sets->SectionSize);
				ULONG zHeadSize = (r + 2) * sizeof(ULONG);
				ULONG * z_list = (ULONG*)malloc(zHeadSize);
				z_list[0] = r;
				//Ϊ��ֹ������ѹ���󷴶��������������Ŀռ�Ӧ������������
				sp = HoldSpace(compressBound((ULONG)fi.data.count) + zHeadSize);

				Shift sptr = mm->GetView(sp);
				if (!_ZipSectionWriteIn(hd, fi.data.count, (BYTE*)sptr.ptr + sptr.offset + zHeadSize, z_list)) {
					ReleaseSpace(sp);
					mm->UnView(sptr);
					return false;
				}
				mm->UnView(sptr);

				fi.data = sp;
				fi.zipedSize = ZIP_INNER;
				newNode.info = fi;

				sp.count = zHeadSize;
				Shift sf = mm->GetView(sp);
				memcpy((BYTE*)sf.ptr + sf.offset, z_list, zHeadSize);
				mm->UnView(sf);

				size_t Real_size = 0;
				for (ULONG x = 1; x < r + 2; x++) {
					Real_size += z_list[x];
				}
				newNode.info.data.count = Real_size;

				sp.location = fi.data.count + Real_size;
				sp.count = fi.data.count - Real_size;
				//�ͷ�������ȴδʹ�õĿռ�
				ReleaseSpace(fi.data);

				free(z_list);
			}
			else {
				Shift sptr = mm->GetView(sp);
				if (!_SectionWriteIn(hd, fi.data.count, sptr)) {
					//����д�벻һ��
					ReleaseSpace(sp);
					mm->UnView(sptr);
					return false;
				}
				mm->UnView(sptr);
				fi.data = sp;
				newNode.info = fi;
			}


		}
		CloseHandle(hd);

		newNode.pat = node;
		newNode.info.type |= INDEX_SELF_MODIFY | FILE_TYPE;
		//���ڵ���������Ϣ
		node->sub->push_back(newNode);
		node->info.type |= INDEX_SUB_SIZECHANGED;

		return true;
	}

	//��һ���ļ��ڵ�ָ�������д��ָ�����ⲿ�ļ���
	void ReadOut(PathNode node, std::string outPath, std::string newName = "") {
		if (!(node.info.type & FILE_TYPE))
		{
			return;
		}
		if (!EndWith(outPath, "\\"))outPath.append("\\");
		MakeOutDirs(outPath, outPath);
		if (newName.empty())outPath.append(node.info.name);
		else outPath.append(newName);
		HANDLE hd = mm->OpenFile((char *)outPath.c_str(), OPEN_ALWAYS);

		if (node.info.type & SEQUENCE_BLOCK) {//�ֿ��ȡ�ļ�
			if (node.sub == NULL) {//�ڵ�δ����
				LoadIndex(&node);//����ڵ�
			}

			for (size_t i = 0; i < node.sub->size(); i++) {
				PathNode * ps = getTargetSequenceNode(&node, i);
				Shift sptr = mm->GetView(ps->info.data);
				_SectionReadOut(hd, ps->info.data.count, sptr);
				mm->UnView(sptr);
			}

		}
		else {
			Shift src;
			if (node.info.zipedSize == ZIP_INNER) {//�ֿ�ѹ��
				src = mm->GetView(node.info.data);
				_ZipSectionReadOut(hd, node.info.data.count, src);
			}
			//else if (node.info.zipedSize > 0) {//�����������������Ϊ WriteIn����ȡ�ֶ�д��ʽ����С���ļ�Ҳ�ᰴZIP_INNER����
			//	SpaceNode zsp = node.info.data;
			//	zsp.count = node.info.zipedSize;
			//	Shift sf = mm->GetView(zsp);
			//	BYTE*  ptr = (BYTE*)malloc(node.info.data.count);
			//	ULONG d_size, r_size,writed=0;
			//	d_size = node.info.data.count;//ԭʼ���ݴ�С
			//	r_size = zsp.count; //ѹ�������ݴ�С
			//	uncompress2(ptr, &d_size, (BYTE*)sf.ptr + sf.offset, &r_size);
			//	WriteFile(hd, ptr, d_size, &writed, NULL);
			//	if (writed != d_size) {
			//		std::cout << " Unzip To File Error" << std::endl;
			//	}
			//}
			else {
				src = mm->GetView(node.info.data);
				_SectionReadOut(hd, node.info.data.count, src);

			}
			mm->UnView(src);
		}

		CloseHandle(hd);
	}

	*/