
#include "DllExport.h"


//Debug x64 为Console调试 Release x64 为生成Dll
BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

//测试用例
int main() {

	FileManager * fm = new FileManager();

	size_t p = 0;
	Setting* sets = new Setting();
	sets->VMSize = 1024 * 1024 * 200;
	//指定文件大小，并按默认设置创建虚拟磁盘文件
	fm->MakeVMFile((char*)"D:\\新建虚拟磁盘.vmf",sets,&p);
	//打开虚拟磁盘文件
	fm->Open("D:\\新建虚拟磁盘.vmf");
	//创建具有指定盘符的分区（就是一个特殊的文件夹）
	fm->MakePartition("新建卷", 'D');
	//通过路径查找指定节点
	PathNode * ps = fm->getNodeByPath("R:\\",NULL);
	//向指定节点下写入外部文件
	fm->ReadFileIn("E:\\Test.pdf",ps,true);
	//向指定节点下写入外部文件夹及其下所有文件
	fm->CopyFloderIn("D:\\客户端\\www",ps,"*",false);
	//以指定节点作为起始节点查找具有指定名称的节点
	std::list<PathNode *> pss = fm->SearchNode("opencl-1.2.pdf", ps);
	//保存所有修改并关闭虚拟磁盘文件
	fm->Close();
	return 0;
}