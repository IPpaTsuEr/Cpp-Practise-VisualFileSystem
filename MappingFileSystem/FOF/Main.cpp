
#include "DllExport.h"


//Debug x64 ΪConsole���� Release x64 Ϊ����Dll
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

//��������
int main() {

	FileManager * fm = new FileManager();

	size_t p = 0;
	Setting* sets = new Setting();
	sets->VMSize = 1024 * 1024 * 200;
	//ָ���ļ���С������Ĭ�����ô�����������ļ�
	fm->MakeVMFile((char*)"D:\\�½��������.vmf",sets,&p);
	//����������ļ�
	fm->Open("D:\\�½��������.vmf");
	//��������ָ���̷��ķ���������һ��������ļ��У�
	fm->MakePartition("�½���", 'D');
	//ͨ��·������ָ���ڵ�
	PathNode * ps = fm->getNodeByPath("R:\\",NULL);
	//��ָ���ڵ���д���ⲿ�ļ�
	fm->ReadFileIn("E:\\Test.pdf",ps,true);
	//��ָ���ڵ���д���ⲿ�ļ��м����������ļ�
	fm->CopyFloderIn("D:\\�ͻ���\\www",ps,"*",false);
	//��ָ���ڵ���Ϊ��ʼ�ڵ���Ҿ���ָ�����ƵĽڵ�
	std::list<PathNode *> pss = fm->SearchNode("opencl-1.2.pdf", ps);
	//���������޸Ĳ��ر���������ļ�
	fm->Close();
	return 0;
}